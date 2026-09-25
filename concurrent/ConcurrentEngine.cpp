#include "concurrent/ConcurrentEngine.h"

#include <algorithm>
#include <stdexcept>

ConcurrentEngine::ConcurrentEngine(int capacidadMemoriaBytes,
                                   SchedulerConfig schedulerConfig)
    : memoria_(capacidadMemoriaBytes),
      scheduler_(schedulerConfig),
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

    if (!memoria_.cargar(proceso)) {
        throw std::runtime_error(
            "Memoria insuficiente para admitir el nuevo proceso");
    }

    Proceso admitido = proceso;
    admitido.estado = Estado::READY;
    admitido.tiempoRestante = admitido.tiempoTotal;
    admitido.tiempoEspera = 0;
    admitido.quantumConsumido = 0;

    colaProcesos_.agregar(admitido);
    workers_[admitido.pid] = std::make_unique<ProcessThread>(admitido.pid);
    trackingIO_[admitido.pid] = InfoIO{};

    registrarEventoInterno(TipoEvento::PROCESS_CREATED, admitido.pid,
                           "Proceso registrado en el motor");
    registrarEventoInterno(TipoEvento::PROCESS_ADMITTED, admitido.pid,
                           "Proceso admitido en memoria simulada");
    registrarEventoInterno(TipoEvento::PROCESS_READY, admitido.pid,
                           "Proceso colocado en cola READY");

    colaPrioridad_ = scheduler_.ordenarColaPrioridad(colaProcesos_.obtenerTodos());

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

    proc->estado = Estado::CANCEL;
    memoria_.liberar(pid);

    auto workerIt = workers_.find(pid);
    if (workerIt != workers_.end()) {
        workerIt->second->solicitarDetencion();
    }

    if (pidEjecutando_ == pid) {
        pidEjecutando_ = -1;
    }

    registrarEventoInterno(TipoEvento::PROCESS_CANCELLED, pid,
                           "Proceso cancelado por accion del usuario");

    colaPrioridad_ = scheduler_.ordenarColaPrioridad(colaProcesos_.obtenerTodos());

    std::vector<Proceso> todos = colaProcesos_.obtenerTodos();
    const bool todosTerminados = std::all_of(
        todos.begin(), todos.end(), [](const Proceso& p) {
            return p.estado == Estado::FINISH || p.estado == Estado::CANCEL;
        });

    if (todosTerminados) {
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
    if (estadoEjecucion_ == EstadoEjecucion::PAUSADO) {
        return;
    }
    estadoEjecucion_ = EstadoEjecucion::PAUSADO;
    cvPasoCompletado_.wait(lock, [this]() {
        return !pasoEnCurso_;
    });
}

void ConcurrentEngine::reiniciar() {
    {
        std::unique_lock<std::mutex> lock(mutexEstado_);
        estadoEjecucion_ = EstadoEjecucion::DETENIDO;
        cvPasoCompletado_.wait(lock, [this]() {
            return !pasoEnCurso_;
        });
    }

    detenerTodosLosWorkersInterno();

    std::lock_guard<std::mutex> lock(mutexEstado_);
    const int capacidad = memoria_.obtenerTotal();
    memoria_ = Memoria(capacidad);
    colaProcesos_ = ColaProcesos{};
    workers_.clear();
    trackingIO_.clear();

    tiempoSimulado_ = 0;
    pidEjecutando_ = -1;
    colaPrioridad_.clear();
    eventos_.clear();
    timeline_.clear();
    estadoEjecucion_ = EstadoEjecucion::DETENIDO;
}

void ConcurrentEngine::siguientePaso() {
    std::unique_lock<std::mutex> lock(mutexEstado_);
    if (estadoEjecucion_ != EstadoEjecucion::PAUSADO) {
        throw std::logic_error(
            "siguientePaso requiere que el motor se encuentre en estado PAUSADO");
    }

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
        cvCoordinador_.wait(lock, [this]() {
            return destruirCoordinador_ ||
                   estadoEjecucion_ == EstadoEjecucion::EJECUTANDO;
        });

        if (destruirCoordinador_) {
            break;
        }

        if (estadoEjecucion_ == EstadoEjecucion::EJECUTANDO) {
            ejecutarPasoInterno(lock);
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
        std::condition_variable& cv;
        GuardPaso(bool& f, std::condition_variable& c) : flag(f), cv(c) {
            flag = true;
        }
        ~GuardPaso() {
            flag = false;
            cv.notify_all();
        }
    } guard(pasoEnCurso_, cvPasoCompletado_);

    std::vector<Proceso> todos = colaProcesos_.obtenerTodos();
    if (todos.empty()) {
        estadoEjecucion_ = EstadoEjecucion::FINALIZADO;
        return;
    }

    bool todosTerminados = std::all_of(
        todos.begin(), todos.end(), [](const Proceso& p) {
            return p.estado == Estado::FINISH || p.estado == Estado::CANCEL;
        });

    if (todosTerminados) {
        estadoEjecucion_ = EstadoEjecucion::FINALIZADO;
        pidEjecutando_ = -1;
        return;
    }

    const int siguientePid = scheduler_.seleccionarSiguiente(todos);

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
            for (auto& p : todos) {
                if (p.estado == Estado::WAIT) {
                    auto& io = trackingIO_[p.pid];
                    io.tiempoEsperaRestanteMs -= menorEsperaWait;
                    if (io.tiempoEsperaRestanteMs <= 0) {
                        Proceso* procMod = colaProcesos_.buscar(p.pid);
                        if (procMod != nullptr) {
                            procMod->estado = Estado::READY;
                            registrarEventoInterno(
                                TipoEvento::PROCESS_READY, p.pid,
                                "Fin de espera E/S, proceso retorna a READY");
                        }
                    }
                }
            }
            colaPrioridad_ =
                scheduler_.ordenarColaPrioridad(colaProcesos_.obtenerTodos());
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

    if (proc->estado == Estado::READY) {
        proc->estado = Estado::RUN;
        pidEjecutando_ = siguientePid;
        registrarEventoInterno(TipoEvento::PROCESS_RUNNING, siguientePid,
                               "Asignacion de CPU al proceso");
        scheduler_.alAsignarCPU(*proc);
    }

    SolicitudTurno solicitud;
    solicitud.cuotaMs = quantumMs_;
    solicitud.tiempoRestante = proc->tiempoRestante;

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

    ProcessThread* worker = nullptr;
    auto workerIt = workers_.find(siguientePid);
    if (workerIt != workers_.end()) {
        worker = workerIt->second.get();
    }

    if (worker == nullptr) {
        return;
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
        colaPrioridad_ =
            scheduler_.ordenarColaPrioridad(colaProcesos_.obtenerTodos());
        return;
    }

    const std::int64_t delta = resultado.tiempoEjecutadoMs;
    if (delta > 0) {
        proc->tiempoRestante -= delta;
        proc->quantumConsumido += delta;

        timeline_.push_back(
            BloqueTimeline{siguientePid, tiempoSimulado_, tiempoSimulado_ + delta});
        tiempoSimulado_ += delta;

        // Avanzar el tiempo de espera de procesos en WAIT
        for (const auto& otra : colaProcesos_.obtenerTodos()) {
            if (otra.estado == Estado::WAIT) {
                auto& io = trackingIO_[otra.pid];
                io.tiempoEsperaRestanteMs -= delta;
                if (io.tiempoEsperaRestanteMs <= 0) {
                    Proceso* enEspera = colaProcesos_.buscar(otra.pid);
                    if (enEspera != nullptr) {
                        enEspera->estado = Estado::READY;
                        registrarEventoInterno(
                            TipoEvento::PROCESS_READY, otra.pid,
                            "Fin de espera E/S, proceso retorna a READY");
                    }
                }
            }
        }

        // Aplicar aging a los demas procesos READY
        std::vector<Proceso> listosParaAging =
            colaProcesos_.obtenerPorEstado(Estado::READY);
        for (auto& listo : listosParaAging) {
            if (listo.pid != siguientePid) {
                const int prioAnterior = listo.prioridadActual;
                std::vector<Proceso> vectorIndividual = {listo};
                scheduler_.aplicarAging(vectorIndividual, delta);
                if (vectorIndividual[0].prioridadActual != prioAnterior) {
                    Proceso* pAging = colaProcesos_.buscar(listo.pid);
                    if (pAging != nullptr) {
                        pAging->prioridadActual =
                            vectorIndividual[0].prioridadActual;
                        pAging->tiempoEspera = vectorIndividual[0].tiempoEspera;
                        registrarEventoInterno(
                            TipoEvento::PRIORITY_CHANGED, listo.pid,
                            "Prioridad incrementada por aging");
                    }
                } else {
                    Proceso* pAging = colaProcesos_.buscar(listo.pid);
                    if (pAging != nullptr) {
                        pAging->tiempoEspera += delta;
                    }
                }
            }
        }
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
        registrarEventoInterno(TipoEvento::PROCESS_WAITING, siguientePid,
                               "Proceso pasa a WAIT por inicio de E/S");
    } else if (resultado.motivo == MotivoFinTurno::QUANTUM_AGOTADO) {
        proc->estado = Estado::READY;
        pidEjecutando_ = -1;
        proc->quantumConsumido = 0;
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

    colaPrioridad_ = scheduler_.ordenarColaPrioridad(colaProcesos_.obtenerTodos());

    std::vector<Proceso> todosFinales = colaProcesos_.obtenerTodos();
    const bool todosTerminadosAlFinal = std::all_of(
        todosFinales.begin(), todosFinales.end(), [](const Proceso& p) {
            return p.estado == Estado::FINISH || p.estado == Estado::CANCEL;
        });

    if (todosTerminadosAlFinal) {
        estadoEjecucion_ = EstadoEjecucion::FINALIZADO;
    }
}

void ConcurrentEngine::registrarEventoInterno(TipoEvento tipo, int pid,
                                             std::string descripcion) {
    Evento evento;
    evento.tiempoSimulado = tiempoSimulado_;
    evento.tipo = tipo;
    evento.pid = pid;
    evento.descripcion = std::move(descripcion);
    eventos_.push_back(std::move(evento));
}

void ConcurrentEngine::detenerTodosLosWorkersInterno() {
    for (auto& par : workers_) {
        if (par.second != nullptr) {
            par.second->solicitarDetencion();
        }
    }
}
