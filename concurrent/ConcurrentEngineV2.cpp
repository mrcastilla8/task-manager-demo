#include "concurrent/ConcurrentEngineV2.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace ConcurrentSimulator::v2 {
namespace {

constexpr ResultadoPaso PASO_INVALIDO{CodigoResultado::ESTADO_INVALIDO, false,
                                      false, 0};

std::int64_t sumaSaturada(std::int64_t a, std::int64_t b) noexcept {
    const std::int64_t maximo = (std::numeric_limits<std::int64_t>::max)();
    return a > maximo - b ? maximo : a + b;
}

}  // namespace

ConcurrentEngineV2::ConcurrentEngineV2(Configuracion configuracion)
    : configuracion_(configuracion),
      memoria_(configuracion.memoriaTotalBytes),
      coordinador_(&ConcurrentEngineV2::loopCoordinador, this) {}

ConcurrentEngineV2::~ConcurrentEngineV2() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        destruir_ = true;
        // Los pasos aun no iniciados terminan como invalidos para que sus
        // llamadores no queden bloqueados.
        for (const auto& solicitud : solicitudes_) {
            solicitud->listo = true;
        }
        solicitudes_.clear();
        // Interrumpe cualquier rafaga retenida y despierta al coordinador.
        for (auto& par : workers_) {
            par.second->solicitarDetencion();
        }
    }
    cvCoordinador_.notify_all();
    cvPaso_.notify_all();
    if (coordinador_.joinable()) {
        coordinador_.join();
    }
}

// ---------------------------------------------------------------------------
// Operaciones publicas
// ---------------------------------------------------------------------------

Resultado ConcurrentEngineV2::agregarProceso(const AltaProceso& alta) {
    std::lock_guard<std::mutex> lock(mutex_);

    switch (admitirProceso(alta, procesos_, memoria_)) {
        case ResultadoAdmision::ALTA_INVALIDA:
            return {CodigoResultado::ARGUMENTO_INVALIDO, alta.pid};
        case ResultadoAdmision::PID_DUPLICADO:
            return {CodigoResultado::PID_DUPLICADO, alta.pid};
        case ResultadoAdmision::MEMORIA_INSUFICIENTE:
            return {CodigoResultado::MEMORIA_INSUFICIENTE, alta.pid};
        case ResultadoAdmision::INSERCION_RECHAZADA:
            return {CodigoResultado::ESTADO_INVALIDO, alta.pid};
        case ResultadoAdmision::ACEPTADO:
            break;
    }

    // El proceso ya esta en la lista y la memoria de Castilla. El alta es
    // transaccional: todo lo que puede fallar ocurre antes de confirmar y, si
    // algo lanza, se retira el proceso, su memoria y su worker.
    const auto deshacer = [this, &alta]() {
        workers_.erase(alta.pid);
        (void)procesos_.eliminar(alta.pid);
        liberarMemoriaInterno(alta.pid);
    };

    std::unique_ptr<::ProcessThread> worker;
    std::vector<Proceso> vista;
    std::vector<Prioridad> anteriores;
    std::uint64_t ticket = 0;
    try {
        inyectarFalloAlta(EtapaAlta::CREAR_WORKER);
        worker = std::make_unique<::ProcessThread>(alta.pid);

        // Recalculo P1/P2/P3 con el Scheduler oficial sobre la lista de Castilla.
        inyectarFalloAlta(EtapaAlta::CLASIFICAR);
        vista = procesos_.obtenerTodos();
        anteriores.reserve(vista.size());
        for (const Proceso& proceso : vista) {
            anteriores.push_back(proceso.prioridad);
        }
        scheduler_.clasificarPrioridades(vista);

        inyectarFalloAlta(EtapaAlta::PREPARAR_EVENTOS);
        reservarEventosInterno(2 + vista.size());

        inyectarFalloAlta(EtapaAlta::REGISTRAR_WORKER);
        workers_.emplace(alta.pid, std::move(worker));

        // Ultimo paso que puede fallar: asignar() no consume ante un overflow.
        inyectarFalloAlta(EtapaAlta::ASIGNAR_TICKET);
        ticket = asignadorTickets_.asignar();
    } catch (const std::overflow_error&) {
        deshacer();
        return {CodigoResultado::ESTADO_INVALIDO, alta.pid};
    } catch (...) {
        deshacer();
        throw;
    }

    // Confirmacion: nada de lo que sigue lanza.
    (void)procesos_.aplicarPrioridades(vista);
    (void)procesos_.actualizarTicketReady(alta.pid, ticket);
    if (estado_ == EstadoEjecucion::FINALIZADO) {
        estado_ = EstadoEjecucion::DETENIDO;
    }
    registrarInterno(TipoEvento::PROCESO_CREADO, alta.pid);
    registrarInterno(TipoEvento::PROCESO_ADMITIDO, alta.pid);
    for (std::size_t indice = 0; indice < vista.size(); ++indice) {
        if (vista[indice].prioridad != anteriores[indice]) {
            registrarInterno(TipoEvento::PRIORIDAD_CAMBIADA, vista[indice].pid);
        }
    }
    return {CodigoResultado::OK, alta.pid};
}

Resultado ConcurrentEngineV2::cancelarProceso(int pid) {
    std::lock_guard<std::mutex> lock(mutex_);

    const Proceso* proceso = procesos_.buscar(pid);
    if (proceso == nullptr) {
        return {CodigoResultado::PID_INEXISTENTE, pid};
    }
    if (proceso->estado == Estado::FINISH) {
        return {CodigoResultado::PID_FINALIZADO, pid};
    }
    if (proceso->estado == Estado::CANCEL) {
        return {CodigoResultado::PID_YA_CANCELADO, pid};
    }

    // Un proceso solo esta en RUN mientras su rafaga esta en curso. La
    // cancelacion no la descarta: se resuelve al cerrarla, tras contabilizar el
    // CPU realmente completado. El protocolo de cancelacion parcial (checkpoints)
    // sigue pendiente de G1 y no se aproxima aqui.
    if (proceso->estado == Estado::RUN) {
        cancelacionesPendientes_.insert(pid);
        return {CodigoResultado::OK, pid};
    }

    reservarEventosInterno(8 + 2 * procesos_.cantidad());
    cancelarInterno(pid);
    return {CodigoResultado::OK, pid};
}

Resultado ConcurrentEngineV2::iniciar() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (estado_ == EstadoEjecucion::FINALIZADO) {
        return {CodigoResultado::ESTADO_INVALIDO, -1};
    }
    // Hasta ratificar G1 no hay ejecucion continua: iniciar() no despacha ni
    // avanza el reloj, igual que el motor logico.
    estado_ = EstadoEjecucion::EJECUTANDO;
    return {CodigoResultado::OK, -1};
}

Resultado ConcurrentEngineV2::pausar() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (estado_ != EstadoEjecucion::EJECUTANDO) {
        return {CodigoResultado::ESTADO_INVALIDO, -1};
    }
    estado_ = EstadoEjecucion::PAUSADO;
    // Punto consistente: no se retorna mientras haya una rafaga en curso.
    cvPaso_.wait(lock, [this]() { return !pasoEnCurso_; });
    return {CodigoResultado::OK, -1};
}

Resultado ConcurrentEngineV2::reiniciar() {
    std::map<int, std::unique_ptr<::ProcessThread>> workersRetirados;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cvPaso_.wait(lock, [this]() { return !reiniciando_; });
        reiniciando_ = true;
        estado_ = EstadoEjecucion::DETENIDO;
        cvPaso_.wait(lock, [this]() { return !pasoEnCurso_; });

        // Todo el estado se limpia sin soltar el mutex: ningun paso puede
        // arrancar (el coordinador respeta reiniciando_) ni usar workers que se
        // estan retirando. La configuracion (memoria total) se conserva.
        workersRetirados.swap(workers_);
        procesos_ = ListaProcesos{};
        memoria_ = Memoria(configuracion_.memoriaTotalBytes);
        asignadorTickets_.reiniciar();
        relojMs_ = 0;
        pidEjecutando_ = -1;
        cuotaTurnoFijadaMs_ = 0;
        cpuConsumidaTurnoMs_ = 0;
        esperas_.clear();
        ioConsumida_.clear();
        cancelacionesPendientes_.clear();
        eventos_.clear();
        timeline_.clear();
        estado_ = EstadoEjecucion::DETENIDO;
        reiniciando_ = false;
    }
    cvPaso_.notify_all();
    cvCoordinador_.notify_all();

    // El join de los workers se hace fuera del mutex de estado.
    for (auto& par : workersRetirados) {
        par.second->solicitarDetencion();
    }
    return {CodigoResultado::OK, -1};
}

ResultadoPaso ConcurrentEngineV2::siguientePaso() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (estado_ != EstadoEjecucion::PAUSADO || destruir_) {
        return PASO_INVALIDO;
    }

    // El coordinador ejecuta los pasos de uno en uno: una unica unidad logica a
    // la vez aunque varios hilos llamen a siguientePaso().
    auto solicitud = std::make_shared<SolicitudPaso>();
    solicitudes_.push_back(solicitud);
    cvCoordinador_.notify_one();
    cvPaso_.wait(lock, [&solicitud]() { return solicitud->listo; });
    return solicitud->resultado;
}

EstadoSistema ConcurrentEngineV2::obtenerEstado() const {
    std::lock_guard<std::mutex> lock(mutex_);
    EstadoSistema estado;
    estado.tiempoSimuladoMs = relojMs_;
    estado.estadoEjecucion = estado_;
    estado.procesos = procesos_.obtenerTodos();
    estado.colaReady = scheduler_.ordenarColaPrioridad(estado.procesos);
    estado.pidEjecutando = pidEjecutando_;
    estado.cuotaTurnoFijadaMs = cuotaTurnoFijadaMs_;
    estado.cpuConsumidaTurnoMs = cpuConsumidaTurnoMs_;
    estado.memoriaUsadaBytes = memoria_.utilizadaBytes();
    estado.memoriaTotalBytes = memoria_.capacidadTotalBytes();
    estado.eventos = eventos_;
    estado.timeline = timeline_;
    return estado;
}

std::size_t ConcurrentEngineV2::cantidadWorkers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return workers_.size();
}

// ---------------------------------------------------------------------------
// Solo pruebas
// ---------------------------------------------------------------------------

void ConcurrentEngineV2::activarPuertaRafagaPruebas(bool activa) {
    std::lock_guard<std::mutex> lock(mutex_);
    puertaActiva_ = activa;
}

void ConcurrentEngineV2::liberarRafagaPruebas() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pidEjecutando_ < 0) {
        return;
    }
    const auto it = workers_.find(pidEjecutando_);
    if (it != workers_.end()) {
        it->second->liberarRafaga();
    }
}

void ConcurrentEngineV2::configurarFalloAltaPruebas(EtapaAlta etapa) {
    std::lock_guard<std::mutex> lock(mutex_);
    falloAltaPruebas_ = etapa;
}

void ConcurrentEngineV2::configurarFalloPasoPruebas(EtapaPasoPrueba etapa) {
    std::lock_guard<std::mutex> lock(mutex_);
    falloPasoPruebas_ = etapa;
}

void ConcurrentEngineV2::forzarMotivoWorkerPruebas(MotivoFinTurno motivo) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pidEjecutando_ < 0) return;
    const auto it = workers_.find(pidEjecutando_);
    if (it != workers_.end() && it->second != nullptr) {
        it->second->forzarMotivoProximoTurnoPruebas(motivo);
    }
}

void ConcurrentEngineV2::fijarSiguienteTicketPruebas(std::uint64_t valor) {
    std::lock_guard<std::mutex> lock(mutex_);
    asignadorTickets_.reiniciar(valor);
}

void ConcurrentEngineV2::inyectarFalloAlta(EtapaAlta etapa) const {
    if (falloAltaPruebas_ == etapa) {
        throw std::runtime_error("Fallo inyectado en el alta (solo pruebas)");
    }
}

void ConcurrentEngineV2::inyectarFalloPaso(EtapaPasoPrueba etapa) {
    if (falloPasoPruebas_ == etapa) {
        falloPasoPruebas_ = EtapaPasoPrueba::NINGUNA;
        throw std::runtime_error("Fallo inyectado durante el paso (solo pruebas)");
    }
}

// ---------------------------------------------------------------------------
// Coordinador y paso
// ---------------------------------------------------------------------------

void ConcurrentEngineV2::loopCoordinador() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (true) {
        cvCoordinador_.wait(lock, [this]() {
            return destruir_ || (!solicitudes_.empty() && !reiniciando_);
        });
        if (destruir_) {
            return;
        }

        const std::shared_ptr<SolicitudPaso> solicitud = solicitudes_.front();
        solicitudes_.pop_front();
        pasoEnCurso_ = true;

        ResultadoPaso resultado = PASO_INVALIDO;
        try {
            resultado = ejecutarPasoInterno(lock);
        } catch (...) {
            // Un fallo de recursos a mitad de paso no debe terminar el hilo.
            if (!lock.owns_lock()) {
                lock.lock();
            }
            abortarTurnoInterno();
            resultado = PASO_INVALIDO;
        }

        pasoEnCurso_ = false;
        solicitud->resultado = resultado;
        solicitud->listo = true;
        cvPaso_.notify_all();
    }
}

void ConcurrentEngineV2::abortarTurnoInterno() noexcept {
    if (pidEjecutando_ >= 0) {
        const int pid = pidEjecutando_;
        (void)procesos_.actualizarEstado(pid, Estado::READY);
        const auto despacho = std::find_if(
            eventos_.rbegin(), eventos_.rend(), [pid](const Evento& evento) {
                return evento.tipo == TipoEvento::READY_A_RUN &&
                       evento.pid == pid;
            });
        if (despacho != eventos_.rend()) {
            eventos_.erase(std::next(despacho).base());
        }
    }
    pidEjecutando_ = -1;
    cuotaTurnoFijadaMs_ = 0;
    cpuConsumidaTurnoMs_ = 0;
}

ResultadoPaso ConcurrentEngineV2::ejecutarPasoInterno(
    std::unique_lock<std::mutex>& lock) {
    if (estado_ != EstadoEjecucion::PAUSADO) {
        return PASO_INVALIDO;
    }

    reservarEventosInterno(16 + 3 * esperas_.size() + 2 * procesos_.cantidad());
    if (!despertarEsperasVencidasInterno()) {
        return PASO_INVALIDO;
    }

    const int pid = scheduler_.seleccionarSiguiente(procesos_.obtenerTodos());
    if (pid < 0) {
        return ejecutarIdleInterno();
    }

    const Proceso* seleccionado = procesos_.buscar(pid);
    const auto workerIt = workers_.find(pid);
    if (seleccionado == nullptr || seleccionado->estado != Estado::READY ||
        workerIt == workers_.end() || workerIt->second == nullptr) {
        return PASO_INVALIDO;
    }
    ::ProcessThread* worker = workerIt->second.get();

    // Datos del turno: la cuota sale de la prioridad vigente y se fija ahora.
    const std::int64_t cuota = cuotaMs(seleccionado->prioridad);
    const std::int64_t restante = seleccionado->tiempoCpuRestanteMs;
    const bool ioPendiente =
        seleccionado->operacionIO.habilitada && ioConsumida_.count(pid) == 0;
    const std::int64_t hastaIO =
        ioPendiente ? seleccionado->operacionIO.despuesDeCpuMs -
                          seleccionado->tiempoCpuConsumidoMs
                    : 0;
    std::int64_t duracion = (std::min)(cuota, restante);
    if (ioPendiente) {
        duracion = (std::min)(duracion, hastaIO);
    }

    const std::int64_t inicio = relojMs_;
    if (duracion <= 0 || !puedeAvanzarRelojInterno(duracion) ||
        duracion > (std::numeric_limits<std::int64_t>::max)() - inicio) {
        return PASO_INVALIDO;
    }
    const std::int64_t finMaximo = inicio + duracion;

    // Reserva exacta de tickets, antes de modificar nada: uno por cada WAIT que
    // despierta durante la rafaga y uno para el propio proceso solo si vuelve a
    // READY por fin de cuota (no si termina ni si pasa a WAIT).
    std::uint64_t ticketsNecesarios = 0;
    for (const Espera& espera : esperas_) {
        if (espera.despertarMs <= finMaximo) {
            const Proceso* esperando = procesos_.buscar(espera.pid);
            if (esperando != nullptr && esperando->estado == Estado::WAIT) {
                ++ticketsNecesarios;
            }
        }
    }
    const bool terminaCPU = duracion == restante;
    const bool terminaPorIO = ioPendiente && duracion == hastaIO;
    if (!terminaCPU && !terminaPorIO) {
        ++ticketsNecesarios;
    }
    if (ticketsNecesarios > ticketsDisponiblesInterno()) {
        return PASO_INVALIDO;
    }
    timeline_.reserve(timeline_.size() + 1);

    // Despacho: pasa a RUN con la cuota fijada.
    if (!procesos_.actualizarEstado(pid, Estado::RUN)) {
        return PASO_INVALIDO;
    }
    pidEjecutando_ = pid;
    cuotaTurnoFijadaMs_ = cuota;
    cpuConsumidaTurnoMs_ = 0;
    registrarInterno(TipoEvento::READY_A_RUN, pid);

    SolicitudTurno solicitud;
    solicitud.cuotaMs = cuota;
    solicitud.tiempoRestante = restante;
    solicitud.tieneIO = ioPendiente;
    solicitud.cpuRestanteParaIO = hastaIO;
    solicitud.esperarLiberacion = puertaActiva_;
    worker->concederTurno(solicitud);

    // El worker calcula la rafaga sin el mutex de estado: snapshots, altas y
    // cancelaciones siguen siendo posibles mientras el proceso esta en RUN.
    lock.unlock();
    const ResultadoRafaga rafaga = worker->esperarFinTurno();
    lock.lock();

    const MotivoFinTurno motivoEsperado = terminaCPU
        ? MotivoFinTurno::PROCESO_TERMINADO
        : (terminaPorIO ? MotivoFinTurno::BLOQUEO_IO
                        : MotivoFinTurno::QUANTUM_AGOTADO);
    // CANCELADO no es un resultado válido del turno calculado: la cancelación
    // RUN provisional se decide al cerrar la ráfaga, no con checkpoints.
    if (rafaga.pid != pid || rafaga.motivo != motivoEsperado ||
        rafaga.tiempoEjecutadoMs != duracion) {
        abortarTurnoInterno();
        return PASO_INVALIDO;
    }

    // Captura bajo mutex después de recoger el worker. Conserva altas y
    // cancelaciones aceptadas mientras aquel calculaba la ráfaga.
    ListaProcesos procesosAntes = procesos_;
    Memoria memoriaAntes = memoria_;
    AsignadorTickets ticketsAntes = asignadorTickets_;
    const std::int64_t relojAntes = relojMs_;
    const EstadoEjecucion estadoAntes = estado_;
    const int pidEjecutandoAntes = pidEjecutando_;
    const std::int64_t cuotaAntes = cuotaTurnoFijadaMs_;
    const std::int64_t cpuTurnoAntes = cpuConsumidaTurnoMs_;
    std::vector<Espera> esperasAntes = esperas_;
    std::unordered_set<int> ioConsumidaAntes = ioConsumida_;
    std::unordered_set<int> cancelacionesAntes = cancelacionesPendientes_;
    const std::size_t cantidadEventosAntes = eventos_.size();
    const std::size_t cantidadBloquesAntes = timeline_.size();
    bool retirarWorker = false;

    try {
        // Se aplica la rafaga por segmentos para despertar a los procesos en WAIT
        // en el instante exacto en que vencen.
        std::int64_t pendiente = duracion;
        while (pendiente > 0) {
            std::int64_t segmento = pendiente;
            for (const Espera& espera : esperas_) {
                if (espera.despertarMs > relojMs_) {
                    segmento = (std::min)(segmento,
                                          espera.despertarMs - relojMs_);
                }
            }
            const Proceso* actual = procesos_.buscar(pid);
            if (actual == nullptr || segmento <= 0 ||
                !puedeAvanzarRelojInterno(segmento) ||
                !procesos_.actualizarCpu(
                    pid, actual->tiempoCpuConsumidoMs + segmento,
                    actual->tiempoCpuRestanteMs - segmento)) {
                throw std::logic_error("No se pudo aplicar un segmento validado");
            }
            relojMs_ += segmento;
            cpuConsumidaTurnoMs_ += segmento;
            pendiente -= segmento;

            for (const Proceso& otro : procesos_.obtenerTodos()) {
                if (otro.estado == Estado::READY && otro.pid != pid) {
                    (void)procesos_.actualizarTiempoEspera(
                        otro.pid, sumaSaturada(otro.tiempoEsperaMs, segmento));
                }
            }
            registrarInterno(TipoEvento::CPU_EJECUTADA, pid, segmento);
            if (!despertarEsperasVencidasInterno()) {
                throw std::logic_error(
                    "Falló un despertar prevalidado del turno");
            }
            inyectarFalloPaso(EtapaPasoPrueba::TRAS_SEGMENTO);
        }

        timeline_.emplace_back(pid, inicio, relojMs_);

        // Transición de fin de ráfaga.
        const Proceso* tras = procesos_.buscar(pid);
        if (tras == nullptr) {
            throw std::logic_error(
                "El proceso RUN desapareció al cerrar el turno");
        }
        if (tras->tiempoCpuRestanteMs == 0) {
            (void)procesos_.actualizarEstado(pid, Estado::FINISH);
            liberarMemoriaInterno(pid);
            registrarInterno(TipoEvento::RUN_A_FINISH, pid);
            reclasificarInterno();
            retirarWorker = true;
        } else if (ioPendiente &&
                   tras->tiempoCpuConsumidoMs ==
                       tras->operacionIO.despuesDeCpuMs) {
            esperas_.push_back(
                {pid, sumaSaturada(relojMs_,
                                  tras->operacionIO.duracionEsperaMs)});
            ioConsumida_.insert(pid);
            (void)procesos_.actualizarEstado(pid, Estado::WAIT);
            registrarInterno(TipoEvento::RUN_A_WAIT, pid);
        } else {
            (void)procesos_.actualizarTicketReady(pid,
                                                 asignadorTickets_.asignar());
            (void)procesos_.actualizarEstado(pid, Estado::READY);
            registrarInterno(TipoEvento::CUOTA_AGOTADA, pid);
            registrarInterno(TipoEvento::RUN_A_READY, pid);
        }

        pidEjecutando_ = -1;
        cuotaTurnoFijadaMs_ = 0;
        cpuConsumidaTurnoMs_ = 0;

        // La cancelación RUN sigue el protocolo provisional: se aplica al
        // completar la ráfaga, después de resolver FINISH/WAIT/READY.
        if (cancelacionesPendientes_.erase(pid) > 0) {
            const Proceso* despues = procesos_.buscar(pid);
            if (despues != nullptr && despues->estado != Estado::FINISH &&
                despues->estado != Estado::CANCEL) {
                cancelarInterno(pid, false);
                retirarWorker = true;
            }
        }

        finalizarSiCorrespondeInterno();
    } catch (...) {
        procesos_ = std::move(procesosAntes);
        memoria_ = std::move(memoriaAntes);
        asignadorTickets_ = ticketsAntes;
        relojMs_ = relojAntes;
        estado_ = estadoAntes;
        pidEjecutando_ = pidEjecutandoAntes;
        cuotaTurnoFijadaMs_ = cuotaAntes;
        cpuConsumidaTurnoMs_ = cpuTurnoAntes;
        esperas_ = std::move(esperasAntes);
        ioConsumida_ = std::move(ioConsumidaAntes);
        cancelacionesPendientes_ = std::move(cancelacionesAntes);
        eventos_.erase(eventos_.begin() + cantidadEventosAntes,
                       eventos_.end());
        timeline_.erase(timeline_.begin() + cantidadBloquesAntes,
                        timeline_.end());
        throw;
    }

    if (retirarWorker) {
        retirarWorkerInterno(pid);
    }
    return {CodigoResultado::OK, true, true, duracion};
}

ResultadoPaso ConcurrentEngineV2::ejecutarIdleInterno() {
    if (esperas_.empty()) {
        finalizarSiCorrespondeInterno();
        return {CodigoResultado::OK, false, false, 0};
    }

    std::int64_t despertar = esperas_.front().despertarMs;
    for (const Espera& espera : esperas_) {
        despertar = (std::min)(despertar, espera.despertarMs);
    }
    const std::int64_t inicio = relojMs_;
    if (despertar > inicio) {
        const std::int64_t delta = despertar - inicio;
        if (!puedeAvanzarRelojInterno(delta)) {
            return PASO_INVALIDO;
        }
        // Reserva de tickets antes de avanzar el reloj: asi un agotamiento no
        // deja un salto idle a medias.
        std::uint64_t despertaran = 0;
        for (const Espera& espera : esperas_) {
            const Proceso* proceso = procesos_.buscar(espera.pid);
            if (espera.despertarMs <= despertar && proceso != nullptr &&
                proceso->estado == Estado::WAIT) {
                ++despertaran;
            }
        }
        if (despertaran > ticketsDisponiblesInterno()) {
            return PASO_INVALIDO;
        }
        registrarInterno(TipoEvento::CPU_IDLE_INICIO);
        relojMs_ += delta;
        registrarInterno(TipoEvento::CPU_IDLE_FIN);
    }
    if (!despertarEsperasVencidasInterno()) {
        return PASO_INVALIDO;
    }
    return {CodigoResultado::OK, true, false, relojMs_ - inicio};
}

// ---------------------------------------------------------------------------
// Helpers (requieren mutex_ tomado)
// ---------------------------------------------------------------------------

bool ConcurrentEngineV2::despertarEsperasVencidasInterno() {
    std::vector<Espera> vencidas;
    for (const Espera& espera : esperas_) {
        const Proceso* proceso = procesos_.buscar(espera.pid);
        if (espera.despertarMs <= relojMs_ && proceso != nullptr &&
            proceso->estado == Estado::WAIT) {
            vencidas.push_back(espera);
        }
    }
    if (vencidas.size() > ticketsDisponiblesInterno()) {
        return false;
    }

    std::vector<std::uint64_t> tickets;
    tickets.reserve(vencidas.size());
    for (std::size_t indice = 0; indice < vencidas.size(); ++indice) {
        tickets.push_back(asignadorTickets_.asignar());
    }
    for (std::size_t indice = 0; indice < vencidas.size(); ++indice) {
        const int pid = vencidas[indice].pid;
        if (!procesos_.actualizarTicketReady(pid, tickets[indice]) ||
            !procesos_.actualizarEstado(pid, Estado::READY)) {
            throw std::logic_error("No se pudo despertar un proceso WAIT");
        }
        registrarInterno(TipoEvento::WAIT_A_READY, pid);
    }
    esperas_.erase(std::remove_if(esperas_.begin(), esperas_.end(),
                                  [this](const Espera& espera) {
                                      return espera.despertarMs <= relojMs_;
                                  }),
                   esperas_.end());
    return true;
}

void ConcurrentEngineV2::cancelarInterno(int pid, bool retirarWorker) {
    (void)procesos_.actualizarEstado(pid, Estado::CANCEL);
    esperas_.erase(std::remove_if(esperas_.begin(), esperas_.end(),
                                  [pid](const Espera& espera) {
                                      return espera.pid == pid;
                                  }),
                   esperas_.end());
    ioConsumida_.erase(pid);
    liberarMemoriaInterno(pid);
    registrarInterno(TipoEvento::PROCESO_CANCELADO, pid);
    reclasificarInterno();
    if (retirarWorker) {
        retirarWorkerInterno(pid);
    }
    finalizarSiCorrespondeInterno();
}

void ConcurrentEngineV2::reclasificarInterno() {
    // DP-02: el Scheduler oficial recalcula P1/P2/P3 y el resultado se guarda en
    // la lista de Castilla; el motor no duplica ni el calculo ni el almacenamiento.
    std::vector<Proceso> vista = procesos_.obtenerTodos();
    std::vector<Prioridad> anteriores;
    anteriores.reserve(vista.size());
    for (const Proceso& proceso : vista) {
        anteriores.push_back(proceso.prioridad);
    }
    scheduler_.clasificarPrioridades(vista);
    if (!procesos_.aplicarPrioridades(vista)) {
        throw std::logic_error(
            "La vista de procesos cambio antes de persistir prioridades");
    }
    for (std::size_t indice = 0; indice < vista.size(); ++indice) {
        if (vista[indice].prioridad != anteriores[indice]) {
            registrarInterno(TipoEvento::PRIORIDAD_CAMBIADA, vista[indice].pid);
        }
    }
}

void ConcurrentEngineV2::registrarInterno(TipoEvento tipo, int pid,
                                          std::int64_t duracionMs) {
    eventos_.push_back({relojMs_, tipo, pid, duracionMs, {}});
}

void ConcurrentEngineV2::reservarEventosInterno(std::size_t adicionales) {
    eventos_.reserve(eventos_.size() + adicionales);
}

void ConcurrentEngineV2::retirarWorkerInterno(int pid) {
    const auto it = workers_.find(pid);
    if (it != workers_.end()) {
        it->second->solicitarDetencion();
        workers_.erase(it);
    }
}

void ConcurrentEngineV2::liberarMemoriaInterno(int pid) noexcept {
    (void)memoria_.liberar(pid);
}

bool ConcurrentEngineV2::hayActivoInterno() const {
    for (const Proceso& proceso : procesos_.obtenerTodos()) {
        if (proceso.estado == Estado::READY || proceso.estado == Estado::RUN ||
            proceso.estado == Estado::WAIT) {
            return true;
        }
    }
    return false;
}

void ConcurrentEngineV2::finalizarSiCorrespondeInterno() {
    if (!hayActivoInterno() && estado_ != EstadoEjecucion::FINALIZADO) {
        estado_ = EstadoEjecucion::FINALIZADO;
        registrarInterno(TipoEvento::SISTEMA_FINALIZADO);
    }
}

std::uint64_t ConcurrentEngineV2::ticketsDisponiblesInterno() const noexcept {
    // El valor maximo nunca se asigna (AsignadorTickets), asi que quedan
    // exactamente max - proximo tickets utilizables.
    return (std::numeric_limits<std::uint64_t>::max)() -
           asignadorTickets_.proximo();
}

bool ConcurrentEngineV2::puedeAvanzarRelojInterno(
    std::int64_t duracionMs) const noexcept {
    return duracionMs >= 0 &&
           duracionMs <= (std::numeric_limits<std::int64_t>::max)() - relojMs_;
}

}  // namespace ConcurrentSimulator::v2
