#include "concurrent/ConcurrentEngine.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

ConcurrentEngine::ConcurrentEngine(int capacidadMemoriaBytes)
    : memoria_(capacidadMemoriaBytes),
      hiloCoordinador_(&ConcurrentEngine::loopCoordinador, this) {}

ConcurrentEngine::~ConcurrentEngine() {
    {
        std::lock_guard<std::mutex> lock(mutexEstado_);
        destruirCoordinador_ = true;
        estadoEjecucion_ = EstadoEjecucion::DETENIDO;
    }
    cvCoordinador_.notify_all();

    if (hiloCoordinador_.joinable()) {
        hiloCoordinador_.join();
    }

    detenerTodosLosWorkersInterno();
}

void ConcurrentEngine::agregarProceso(const Proceso& proceso) {
    std::lock_guard<std::mutex> lock(mutexEstado_);

    if (!procesoValido(proceso)) {
        throw std::invalid_argument("El proceso suministrado no es valido");
    }
    if (colaProcesos_.buscar(proceso.pid) != nullptr) {
        throw std::invalid_argument("Ya existe un proceso con el mismo PID");
    }
    exigirTicketsDisponiblesInterno(1);

    Proceso admitido = proceso;
    admitido.estado = Estado::READY;
    admitido.tiempoRestante = admitido.tiempoTotal;
    admitido.tiempoEspera = 0;
    admitido.quantumConsumido = 0;
    admitido.ordenLlegada = siguienteTicket_;  // se consume al confirmar el alta

    // El alta es transaccional: todo lo que puede fallar (hilo, memoria,
    // insercion, calculo de la cola y eventos) ocurre antes de confirmar y se
    // deshace si algo lanza. Ningun error deja memoria reservada, procesos
    // incompletos ni workers inconsistentes.
    inyectarFalloAlta(EtapaAlta::CREAR_WORKER);
    std::unique_ptr<ProcessThread> worker =
        std::make_unique<ProcessThread>(admitido.pid);

    if (!memoria_.cargar(proceso)) {
        throw std::runtime_error(
            "Memoria insuficiente para admitir el nuevo proceso");
    }

    std::vector<int> nuevaCola;
    std::array<Evento, 3> nuevosEventos;
    try {
        inyectarFalloAlta(EtapaAlta::INSERTAR_COLA);
        colaProcesos_.agregar(admitido);
        try {
            inyectarFalloAlta(EtapaAlta::REGISTRAR_WORKER);
            workers_[admitido.pid] = std::move(worker);
            trackingIO_[admitido.pid] = InfoIO{};

            inyectarFalloAlta(EtapaAlta::CALCULAR_COLA);
            nuevaCola =
                planificador_.ordenarCola(colaProcesos_.obtenerTodos());

            inyectarFalloAlta(EtapaAlta::PREPARAR_EVENTOS);
            nuevosEventos = {
                construirEvento(TipoEvento::PROCESS_CREATED, admitido.pid,
                                "Proceso registrado en el motor"),
                construirEvento(TipoEvento::PROCESS_ADMITTED, admitido.pid,
                                "Proceso admitido en memoria simulada"),
                construirEvento(TipoEvento::PROCESS_READY, admitido.pid,
                                "Proceso colocado en cola READY")};
            eventos_.reserve(eventos_.size() + nuevosEventos.size());
        } catch (...) {
            trackingIO_.erase(admitido.pid);
            workers_.erase(admitido.pid);  // el destructor une el hilo
            colaProcesos_.eliminar(admitido.pid);
            throw;
        }
    } catch (...) {
        memoria_.liberar(admitido.pid);
        throw;
    }

    // Confirmacion: a partir de aqui nada puede lanzar.
    asignarTicketInterno();
    colaPrioridad_ = std::move(nuevaCola);
    for (auto& evento : nuevosEventos) {
        eventos_.push_back(std::move(evento));
    }

    if (estadoEjecucion_ == EstadoEjecucion::FINALIZADO) {
        estadoEjecucion_ = EstadoEjecucion::PAUSADO;
    }
}

void ConcurrentEngine::cancelarProceso(int pid) {
    std::lock_guard<std::mutex> lock(mutexEstado_);

    Proceso* proc = colaProcesos_.buscar(pid);
    if (proc == nullptr || proc->estado == Estado::CANCEL ||
        proc->estado == Estado::FINISH) {
        return;
    }

    // Un proceso solo esta en RUN mientras su rafaga esta en curso. La
    // cancelacion no descarta esa rafaga: se resuelve al cerrarla, despues de
    // contabilizar el CPU realmente completado.
    if (proc->estado == Estado::RUN) {
        cancelacionesPendientes_.insert(pid);
        return;
    }

    cancelarProcesoInterno(*proc);
    actualizarColaPrioridadInterno();

    if (todosTerminadosInterno()) {
        estadoEjecucion_ = EstadoEjecucion::FINALIZADO;
    }
}

void ConcurrentEngine::iniciar() {
    {
        std::lock_guard<std::mutex> lock(mutexEstado_);
        if (estadoEjecucion_ == EstadoEjecucion::EJECUTANDO) {
            return;
        }
        estadoEjecucion_ = EstadoEjecucion::EJECUTANDO;
    }
    cvCoordinador_.notify_all();
}

void ConcurrentEngine::pausar() {
    std::unique_lock<std::mutex> lock(mutexEstado_);
    if (estadoEjecucion_ != EstadoEjecucion::PAUSADO) {
        estadoEjecucion_ = EstadoEjecucion::PAUSADO;
    }
    // Punto consistente: no se retorna mientras haya una rafaga en curso.
    cvPasoCompletado_.wait(lock, [this]() {
        return !pasoEnCurso_;
    });
}

void ConcurrentEngine::reiniciar() {
    std::unordered_map<int, std::unique_ptr<ProcessThread>> workersRetirados;
    {
        std::unique_lock<std::mutex> lock(mutexEstado_);
        cvPasoCompletado_.wait(lock, [this]() {
            return !reiniciando_;
        });
        reiniciando_ = true;
        estadoEjecucion_ = EstadoEjecucion::DETENIDO;
        cvPasoCompletado_.wait(lock, [this]() {
            return !pasoEnCurso_;
        });

        // Todo el estado se limpia sin soltar el mutex: ningun paso puede
        // arrancar (el coordinador respeta reiniciando_) ni usar workers
        // que se estan retirando.
        const int capacidad = memoria_.obtenerTotal();
        workersRetirados.swap(workers_);
        memoria_ = Memoria(capacidad);
        colaProcesos_ = ColaProcesos{};
        trackingIO_.clear();
        cancelacionesPendientes_.clear();
        siguienteTicket_ = 1;

        tiempoSimulado_ = 0;
        pidEjecutando_ = -1;
        colaPrioridad_.clear();
        eventos_.clear();
        timeline_.clear();
        estadoEjecucion_ = EstadoEjecucion::DETENIDO;
        reiniciando_ = false;
    }
    cvPasoCompletado_.notify_all();
    cvCoordinador_.notify_all();

    // El join de los workers se hace fuera del mutex de estado.
    for (auto& par : workersRetirados) {
        if (par.second != nullptr) {
            par.second->solicitarDetencion();
        }
    }
}

void ConcurrentEngine::siguientePaso() {
    std::unique_lock<std::mutex> lock(mutexEstado_);
    const auto exigirPausado = [this]() {
        if (estadoEjecucion_ != EstadoEjecucion::PAUSADO) {
            throw std::logic_error(
                "siguientePaso requiere que el motor se encuentre en estado "
                "PAUSADO");
        }
    };
    exigirPausado();

    // Una unica unidad logica a la vez: si otro hilo ejecuta un paso, se espera
    // a que cierre y se vuelve a validar el estado (pudo cambiar mientras tanto).
    cvPasoCompletado_.wait(lock, [this]() {
        return !pasoEnCurso_;
    });
    exigirPausado();

    ejecutarPasoInterno(lock);
}

void ConcurrentEngine::configurarQuantum(std::int64_t quantumMs) {
    if (quantumMs <= 0) {
        throw std::invalid_argument("El quantum debe ser un valor positivo");
    }
    std::lock_guard<std::mutex> lock(mutexEstado_);
    quantumMs_ = quantumMs;
}

std::int64_t ConcurrentEngine::obtenerQuantum() const {
    std::lock_guard<std::mutex> lock(mutexEstado_);
    return quantumMs_;
}

EstadoSistema ConcurrentEngine::obtenerEstado() const {
    std::lock_guard<std::mutex> lock(mutexEstado_);

    EstadoSistema snapshot;
    snapshot.tiempoSimulado = tiempoSimulado_;
    snapshot.estadoEjecucion = estadoEjecucion_;
    snapshot.quantumMs = quantumMs_;
    snapshot.procesos = colaProcesos_.obtenerTodos();
    snapshot.colaPrioridad = colaPrioridad_;
    snapshot.pidEjecutando = pidEjecutando_;
    snapshot.memoriaUsada = memoria_.obtenerUtilizada();
    snapshot.memoriaTotal = memoria_.obtenerTotal();
    snapshot.eventos = eventos_;
    snapshot.timeline = timeline_;

    return snapshot;
}

void ConcurrentEngine::fijarSiguienteTicketPruebas(std::int64_t valor) {
    std::lock_guard<std::mutex> lock(mutexEstado_);
    siguienteTicket_ = valor;
}

std::int64_t ConcurrentEngine::asignarTicketInterno() {
    // El valor maximo nunca se asigna: asi el contador no desborda (misma regla
    // que AsignadorTickets de Brickzon).
    if (siguienteTicket_ >= std::numeric_limits<std::int64_t>::max()) {
        throw std::overflow_error("Contador de tickets READY agotado");
    }
    return siguienteTicket_++;
}

void ConcurrentEngine::exigirTicketsDisponiblesInterno(
    std::int64_t cantidad) const {
    if (siguienteTicket_ > std::numeric_limits<std::int64_t>::max() - cantidad) {
        throw std::overflow_error("Contador de tickets READY agotado");
    }
}

void ConcurrentEngine::configurarFalloAltaPruebas(EtapaAlta etapa) {
    std::lock_guard<std::mutex> lock(mutexEstado_);
    falloAltaPruebas_ = etapa;
}

void ConcurrentEngine::inyectarFalloAlta(EtapaAlta etapa) const {
    if (falloAltaPruebas_ == etapa) {
        throw std::runtime_error("Fallo inyectado en el alta (solo pruebas)");
    }
}

void ConcurrentEngine::configurarRetardoRafagaPruebasMs(int ms) noexcept {
    retardoRafagaPruebasMs_ = std::max(0, ms);
}

void ConcurrentEngine::configurarIntervaloCoordinadorMs(int ms) noexcept {
    intervaloCoordinadorMs_ = std::max(0, ms);
}

int ConcurrentEngine::obtenerIntervaloCoordinadorMs() const noexcept {
    return intervaloCoordinadorMs_;
}

int ConcurrentEngine::memoriaTotal() const noexcept {
    std::lock_guard<std::mutex> lock(mutexEstado_);
    return memoria_.obtenerTotal();
}

int ConcurrentEngine::memoriaUsada() const noexcept {
    std::lock_guard<std::mutex> lock(mutexEstado_);
    return memoria_.obtenerUtilizada();
}

int ConcurrentEngine::cantidadWorkersActivos() const {
    std::lock_guard<std::mutex> lock(mutexEstado_);
    int activos = 0;
    for (const auto& par : workers_) {
        if (par.second != nullptr && !par.second->estaDetenido()) {
            ++activos;
        }
    }
    return activos;
}

void ConcurrentEngine::loopCoordinador() {
    while (true) {
        std::unique_lock<std::mutex> lock(mutexEstado_);
        // Solo se inicia una unidad si no hay otra en curso (por ejemplo un
        // siguientePaso() de otro hilo) y no se esta reiniciando.
        cvCoordinador_.wait(lock, [this]() {
            return destruirCoordinador_ ||
                   (estadoEjecucion_ == EstadoEjecucion::EJECUTANDO &&
                    !pasoEnCurso_ && !reiniciando_);
        });

        if (destruirCoordinador_) {
            break;
        }

        try {
            ejecutarPasoInterno(lock);
        } catch (const std::exception&) {
            // Por ejemplo, contador de tickets agotado: el paso no modifico nada.
            // El motor se pausa en lugar de terminar el hilo coordinador; un
            // siguientePaso() manual reproduce y notifica la excepcion.
            if (!lock.owns_lock()) {
                lock.lock();
            }
            estadoEjecucion_ = EstadoEjecucion::PAUSADO;
        }

        const int esperaMs = intervaloCoordinadorMs_;
        if (esperaMs > 0 && estadoEjecucion_ == EstadoEjecucion::EJECUTANDO &&
            !destruirCoordinador_) {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(esperaMs));
        }
    }
}

void ConcurrentEngine::ejecutarPasoInterno(std::unique_lock<std::mutex>& lock) {
    struct GuardPaso {
        bool& flag;
        std::condition_variable& cvPaso;
        std::condition_variable& cvCoord;
        GuardPaso(bool& f, std::condition_variable& paso,
                  std::condition_variable& coord)
            : flag(f), cvPaso(paso), cvCoord(coord) {
            flag = true;
        }
        ~GuardPaso() {
            flag = false;
            cvPaso.notify_all();
            cvCoord.notify_all();
        }
    } guard(pasoEnCurso_, cvPasoCompletado_, cvCoordinador_);

    std::vector<Proceso> todos = colaProcesos_.obtenerTodos();
    if (todos.empty()) {
        estadoEjecucion_ = EstadoEjecucion::FINALIZADO;
        return;
    }

    if (todosTerminadosInterno()) {
        estadoEjecucion_ = EstadoEjecucion::FINALIZADO;
        pidEjecutando_ = -1;
        return;
    }

    // Un paso consume como maximo un ticket para el proceso en ejecucion (bloqueo
    // o fin de cuota) y uno por cada proceso en WAIT que pueda despertar.
    // Se comprueba antes de modificar nada para no dejar el paso a medias.
    exigirTicketsDisponiblesInterno(
        1 + static_cast<std::int64_t>(
                colaProcesos_.obtenerPorEstado(Estado::WAIT).size()));

    const int siguientePid = planificador_.seleccionarSiguiente(todos);

    // Caso 1: Ningun proceso en READY, pero podria haber procesos en WAIT
    if (siguientePid == -1) {
        std::int64_t menorEsperaWait = -1;
        for (const auto& p : todos) {
            if (p.estado == Estado::WAIT) {
                const auto it = trackingIO_.find(p.pid);
                if (it != trackingIO_.end()) {
                    if (menorEsperaWait < 0 ||
                        it->second.tiempoEsperaRestanteMs < menorEsperaWait) {
                        menorEsperaWait = it->second.tiempoEsperaRestanteMs;
                    }
                }
            }
        }

        if (menorEsperaWait > 0) {
            tiempoSimulado_ += menorEsperaWait;
            avanzarEsperasIOInterno(menorEsperaWait);
            actualizarColaPrioridadInterno();
        } else {
            estadoEjecucion_ = EstadoEjecucion::FINALIZADO;
        }
        return;
    }

    // Caso 2: Proceso READY seleccionado por el Scheduler
    Proceso* proc = colaProcesos_.buscar(siguientePid);
    if (proc == nullptr) {
        return;
    }

    // El worker se localiza antes de marcar RUN: un proceso nunca debe quedar en
    // RUN sin hilo que ejecute su rafaga (bloquearia el CPU logico).
    const auto workerIt = workers_.find(siguientePid);
    if (workerIt == workers_.end() || workerIt->second == nullptr) {
        cancelarProcesoInterno(
            *proc, "Proceso sin worker asociado; cancelado para no bloquear la CPU");
        actualizarColaPrioridadInterno();
        if (todosTerminadosInterno()) {
            estadoEjecucion_ = EstadoEjecucion::FINALIZADO;
        }
        return;
    }
    ProcessThread* worker = workerIt->second.get();

    if (proc->estado == Estado::READY) {
        proc->estado = Estado::RUN;
        pidEjecutando_ = siguientePid;
        registrarEventoInterno(TipoEvento::PROCESS_RUNNING, siguientePid,
                               "Asignacion de CPU al proceso");
        planificador_.alAsignarCPU(*proc);
        // Mientras el worker ejecuta la rafaga el mutex queda libre y la GUI
        // puede leer el snapshot: la cola READY ya no debe incluir al proceso RUN.
        actualizarColaPrioridadInterno();
    }

    SolicitudTurno solicitud;
    solicitud.cuotaMs = cuotaParaDespachoInterno(*proc);
    solicitud.tiempoRestante = proc->tiempoRestante;
    solicitud.retardoRealMs = retardoRafagaPruebasMs_;

    auto& infoIO = trackingIO_[siguientePid];
    if (proc->operacionIO.habilitada && !infoIO.operacionRealizada) {
        const std::int64_t cpuEjecutada =
            proc->tiempoTotal - proc->tiempoRestante;
        if (cpuEjecutada < proc->operacionIO.despuesDeCpuMs) {
            solicitud.tieneIO = true;
            solicitud.cpuRestanteParaIO =
                proc->operacionIO.despuesDeCpuMs - cpuEjecutada;
        }
    }

    worker->concederTurno(solicitud);

    // Liberamos el cerrojo del estado mientras el worker real ejecuta la rafaga
    lock.unlock();
    ResultadoRafaga resultado = worker->esperarFinTurno();
    lock.lock();

    // Reobtenemos el proceso de forma segura tras readquirir el mutex
    proc = colaProcesos_.buscar(siguientePid);
    if (proc == nullptr || proc->estado == Estado::CANCEL) {
        pidEjecutando_ = -1;
        actualizarColaPrioridadInterno();
        return;
    }

    const std::int64_t delta = resultado.tiempoEjecutadoMs;
    if (delta > 0) {
        proc->tiempoRestante -= delta;
        proc->quantumConsumido += delta;

        timeline_.push_back(
            BloqueTimeline{siguientePid, tiempoSimulado_, tiempoSimulado_ + delta});
        tiempoSimulado_ += delta;

        avanzarEsperasIOInterno(delta);
        acumularEsperaListosInterno(delta);
    }

    // Resolucion de transicion de fin de rafaga
    if (resultado.motivo == MotivoFinTurno::PROCESO_TERMINADO ||
        proc->tiempoRestante <= 0) {
        proc->tiempoRestante = 0;
        proc->estado = Estado::FINISH;
        pidEjecutando_ = -1;
        memoria_.liberar(siguientePid);
        registrarEventoInterno(TipoEvento::PROCESS_FINISHED, siguientePid,
                               "Proceso ha completado su tiempo total");
    } else if (resultado.motivo == MotivoFinTurno::BLOQUEO_IO) {
        proc->estado = Estado::WAIT;
        pidEjecutando_ = -1;
        trackingIO_[siguientePid].operacionRealizada = true;
        trackingIO_[siguientePid].tiempoEsperaRestanteMs =
            proc->operacionIO.duracionEsperaMs;
        proc->quantumConsumido = 0;
        proc->ordenLlegada = asignarTicketInterno();  // ticket de bloqueo
        registrarEventoInterno(TipoEvento::PROCESS_WAITING, siguientePid,
                               "Proceso pasa a WAIT por inicio de E/S");
    } else if (resultado.motivo == MotivoFinTurno::QUANTUM_AGOTADO) {
        proc->estado = Estado::READY;
        pidEjecutando_ = -1;
        proc->quantumConsumido = 0;
        proc->ordenLlegada = asignarTicketInterno();  // Round Robin: ticket nuevo
        registrarEventoInterno(TipoEvento::QUANTUM_FINISHED, siguientePid,
                               "Proceso agoto su quantum asignado");
        registrarEventoInterno(TipoEvento::PROCESS_READY, siguientePid,
                               "Proceso regresa a cola READY");
    } else if (resultado.motivo == MotivoFinTurno::CANCELADO) {
        proc->estado = Estado::CANCEL;
        pidEjecutando_ = -1;
        memoria_.liberar(siguientePid);
        registrarEventoInterno(TipoEvento::PROCESS_CANCELLED, siguientePid,
                               "Proceso cancelado durante la ejecucion");
    }

    // Cancelacion solicitada durante la rafaga: se aplica sobre el estado ya
    // resuelto. Si el proceso termino (FINISH) en este mismo limite, FINISH gana.
    if (cancelacionesPendientes_.erase(siguientePid) > 0 &&
        proc->estado != Estado::FINISH && proc->estado != Estado::CANCEL) {
        cancelarProcesoInterno(*proc);
    }

    actualizarColaPrioridadInterno();

    if (todosTerminadosInterno()) {
        estadoEjecucion_ = EstadoEjecucion::FINALIZADO;
    }
}

void ConcurrentEngine::cancelarProcesoInterno(Proceso& proceso,
                                              const char* descripcion) {
    proceso.estado = Estado::CANCEL;
    memoria_.liberar(proceso.pid);

    const auto workerIt = workers_.find(proceso.pid);
    if (workerIt != workers_.end()) {
        workerIt->second->solicitarDetencion();
    }

    if (pidEjecutando_ == proceso.pid) {
        pidEjecutando_ = -1;
    }

    registrarEventoInterno(TipoEvento::PROCESS_CANCELLED, proceso.pid,
                           descripcion);
}

bool ConcurrentEngine::todosTerminadosInterno() const {
    const std::vector<Proceso> todos = colaProcesos_.obtenerTodos();
    return std::all_of(todos.begin(), todos.end(), [](const Proceso& p) {
        return p.estado == Estado::FINISH || p.estado == Estado::CANCEL;
    });
}

void ConcurrentEngine::actualizarColaPrioridadInterno() {
    colaPrioridad_ = planificador_.ordenarCola(colaProcesos_.obtenerTodos());
}

void ConcurrentEngine::avanzarEsperasIOInterno(std::int64_t delta) {
    std::vector<int> despertados;
    for (const auto& p : colaProcesos_.obtenerPorEstado(Estado::WAIT)) {
        auto& io = trackingIO_[p.pid];
        io.tiempoEsperaRestanteMs -= delta;
        if (io.tiempoEsperaRestanteMs <= 0) {
            despertados.push_back(p.pid);
        }
    }

    // Orden determinista de reingreso: instante de despertar (un restante mas
    // negativo despierto antes), ticket asignado al bloquear y PID.
    std::sort(despertados.begin(), despertados.end(), [this](int a, int b) {
        const auto restanteA = trackingIO_[a].tiempoEsperaRestanteMs;
        const auto restanteB = trackingIO_[b].tiempoEsperaRestanteMs;
        if (restanteA != restanteB) {
            return restanteA < restanteB;
        }
        const auto ticketA = colaProcesos_.buscar(a)->ordenLlegada;
        const auto ticketB = colaProcesos_.buscar(b)->ordenLlegada;
        if (ticketA != ticketB) {
            return ticketA < ticketB;
        }
        return a < b;
    });

    for (int pid : despertados) {
        Proceso* enEspera = colaProcesos_.buscar(pid);
        if (enEspera != nullptr) {
            enEspera->estado = Estado::READY;
            enEspera->ordenLlegada = asignarTicketInterno();
            registrarEventoInterno(
                TipoEvento::PROCESS_READY, pid,
                "Fin de espera E/S, proceso retorna a READY");
        }
    }
}

void ConcurrentEngine::acumularEsperaListosInterno(std::int64_t delta) {
    // DP-10: no hay aging. El tiempo de espera es solo una metrica y no modifica
    // la prioridad. Consecuencia aceptada (propuesta tecnica 8): un proceso de
    // baja prioridad puede esperar indefinidamente mientras haya READY de mayor
    // prioridad.
    for (const auto& listo : colaProcesos_.obtenerPorEstado(Estado::READY)) {
        Proceso* proceso = colaProcesos_.buscar(listo.pid);
        if (proceso != nullptr) {
            proceso->tiempoEspera += delta;
        }
    }
}

std::int64_t ConcurrentEngine::cuotaParaDespachoInterno(
    const Proceso& /*proceso*/) const noexcept {
    // Unico punto donde se decide la cuota del turno; se copia a la solicitud al
    // despachar y no cambia durante la rafaga (DP-04). Hoy es el quantum global
    // de v1. Al migrar a v2 devolvera cuotaMs(prioridad): P1=10, P2=20, P3=30 ms.
    return quantumMs_;
}

Evento ConcurrentEngine::construirEvento(TipoEvento tipo, int pid,
                                         std::string descripcion) const {
    Evento evento;
    evento.tiempoSimulado = tiempoSimulado_;
    evento.tipo = tipo;
    evento.pid = pid;
    evento.descripcion = std::move(descripcion);
    return evento;
}

void ConcurrentEngine::registrarEventoInterno(TipoEvento tipo, int pid,
                                             std::string descripcion) {
    eventos_.push_back(construirEvento(tipo, pid, std::move(descripcion)));
}

void ConcurrentEngine::detenerTodosLosWorkersInterno() {
    for (auto& par : workers_) {
        if (par.second != nullptr) {
            par.second->solicitarDetencion();
        }
    }
}
