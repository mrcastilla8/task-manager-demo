#include "concurrent/ConcurrentEngineV2.h"

#include "scheduler/SchedulerV2.h"

#include <algorithm>
#include <stdexcept>

namespace ConcurrentSimulator::v2 {

ConcurrentEngineV2::ConcurrentEngineV2(Configuracion configuracion)
    : motor_(configuracion), coordinador_(&ConcurrentEngineV2::loopCoordinador,
                                          this) {}

ConcurrentEngineV2::~ConcurrentEngineV2() {
    try {
        encolar([this]() { detenerWorkers(); });
    } catch (...) {
    }
    {
        std::lock_guard<std::mutex> lock(mutexCola_);
        detenerCoordinador_ = true;
    }
    cvTrabajos_.notify_all();
    if (coordinador_.joinable()) {
        coordinador_.join();
    }
}

Resultado ConcurrentEngineV2::agregarProceso(const AltaProceso& proceso) {
    return encolar([this, proceso]() {
        auto worker = std::make_unique<::ProcessThread>(proceso.pid);
        const auto insercion = workers_.emplace(proceso.pid, std::move(worker));
        if (!insercion.second) {
            return Resultado{CodigoResultado::PID_DUPLICADO, proceso.pid};
        }
        Resultado resultado;
        try {
            resultado = motor_.agregarProceso(proceso);
        } catch (...) {
            insercion.first->second->solicitarDetencion();
            workers_.erase(insercion.first);
            throw;
        }
        if (!resultado) {
            insercion.first->second->solicitarDetencion();
            workers_.erase(insercion.first);
        }
        return resultado;
    });
}

Resultado ConcurrentEngineV2::cancelarProceso(int pid) {
    return encolar([this, pid]() {
        const Resultado resultado = motor_.cancelarProceso(pid);
        if (resultado) {
            const auto worker = workers_.find(pid);
            if (worker != workers_.end()) {
                worker->second->solicitarDetencion();
                workers_.erase(worker);
            }
        }
        return resultado;
    });
}

Resultado ConcurrentEngineV2::iniciar() {
    return encolar([this]() { return motor_.iniciar(); });
}

Resultado ConcurrentEngineV2::pausar() {
    return encolar([this]() { return motor_.pausar(); });
}

Resultado ConcurrentEngineV2::reiniciar() {
    return encolar([this]() {
        detenerWorkers();
        return motor_.reiniciar();
    });
}

ResultadoPaso ConcurrentEngineV2::siguientePaso() {
    return encolar([this]() { return siguientePasoInterno(); });
}

EstadoSistema ConcurrentEngineV2::obtenerEstado() const {
    return encolar([this]() { return motor_.obtenerEstado(); });
}

std::size_t ConcurrentEngineV2::cantidadWorkers() const {
    return encolar([this]() { return workers_.size(); });
}

void ConcurrentEngineV2::loopCoordinador() {
    while (true) {
        std::function<void()> trabajo;
        {
            std::unique_lock<std::mutex> lock(mutexCola_);
            cvTrabajos_.wait(lock, [this]() {
                return detenerCoordinador_ || !trabajos_.empty();
            });
            if (trabajos_.empty() && detenerCoordinador_) {
                return;
            }
            trabajo = std::move(trabajos_.front());
            trabajos_.pop();
        }
        trabajo();
    }
}

ResultadoPaso ConcurrentEngineV2::siguientePasoInterno() {
    const EstadoSistema antes = motor_.obtenerEstado();
    if (antes.estadoEjecucion != EstadoEjecucion::PAUSADO ||
        antes.colaReady.empty()) {
        return motor_.siguientePaso();
    }

    const int pid = antes.colaReady.front();
    const auto procesoIt = std::find_if(
        antes.procesos.begin(), antes.procesos.end(),
        [pid](const Proceso& proceso) { return proceso.pid == pid; });
    const auto workerIt = workers_.find(pid);
    if (procesoIt == antes.procesos.end() || workerIt == workers_.end()) {
        return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
    }

    const Proceso& proceso = *procesoIt;
    SolicitudTurno solicitud;
    solicitud.cuotaMs = cuotaMs(proceso.prioridad);
    solicitud.tiempoRestante = proceso.tiempoCpuRestanteMs;
    solicitud.tieneIO = proceso.operacionIO.habilitada &&
                        !IOConsumida(pid, antes);
    if (solicitud.tieneIO) {
        solicitud.cpuRestanteParaIO =
            proceso.operacionIO.despuesDeCpuMs -
            proceso.tiempoCpuConsumidoMs;
    }

    workerIt->second->concederTurno(solicitud);
    const ResultadoRafaga rafaga = workerIt->second->esperarFinTurno();
    if (rafaga.pid != pid || rafaga.tiempoEjecutadoMs <= 0) {
        return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
    }

    const ResultadoPaso paso = motor_.siguientePaso();
    if (paso.codigo != CodigoResultado::OK || !paso.ejecutoCpu ||
        paso.tiempoAvanzadoMs != rafaga.tiempoEjecutadoMs) {
        return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
    }

    const EstadoSistema despues = motor_.obtenerEstado();
    const auto actualizado = std::find_if(
        despues.procesos.begin(), despues.procesos.end(),
        [pid](const Proceso& candidato) { return candidato.pid == pid; });
    if (actualizado != despues.procesos.end() &&
        actualizado->estado == Estado::FINISH) {
        workerIt->second->solicitarDetencion();
        workers_.erase(workerIt);
    }
    return paso;
}

bool ConcurrentEngineV2::IOConsumida(int pid,
                                     const EstadoSistema& estado) const {
    return std::any_of(estado.eventos.begin(), estado.eventos.end(),
                       [pid](const Evento& evento) {
                           return evento.pid == pid &&
                                  evento.tipo == TipoEvento::RUN_A_WAIT;
                       });
}

void ConcurrentEngineV2::detenerWorkers() {
    for (auto& entrada : workers_) {
        entrada.second->solicitarDetencion();
    }
    workers_.clear();
}

}  // namespace ConcurrentSimulator::v2
