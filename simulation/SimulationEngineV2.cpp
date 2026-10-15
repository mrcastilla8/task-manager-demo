#include "simulation/SimulationEngineV2.h"

#include <algorithm>
#include <utility>

namespace ConcurrentSimulator::v2 {

SimulationEngineV2::SimulationEngineV2(Configuracion configuracion)
    : configuracion_(configuracion) {}

Resultado SimulationEngineV2::agregarProceso(const AltaProceso& alta) {
    if (alta.pid <= 0 || alta.nombre.empty() || alta.tamanioBytes <= 0 ||
        alta.tiempoCpuTotalMs <= 0 ||
        (alta.operacionIO.habilitada &&
         (alta.operacionIO.despuesDeCpuMs <= 0 ||
          alta.operacionIO.despuesDeCpuMs >= alta.tiempoCpuTotalMs ||
          alta.operacionIO.duracionEsperaMs <= 0))) {
        return {CodigoResultado::ARGUMENTO_INVALIDO, alta.pid};
    }
    if (buscarProceso(alta.pid) != nullptr) {
        return {CodigoResultado::PID_DUPLICADO, alta.pid};
    }
    if (configuracion_.memoriaTotalBytes < 0 ||
        alta.tamanioBytes > configuracion_.memoriaTotalBytes - memoriaUsadaBytes_) {
        return {CodigoResultado::MEMORIA_INSUFICIENTE, alta.pid};
    }

    Proceso proceso{alta.pid, alta.nombre, alta.tamanioBytes, Prioridad::P3,
                     alta.tiempoCpuTotalMs, alta.tiempoCpuTotalMs, 0, 0, 0,
                     Estado::READY, alta.operacionIO};
    if (!asignarTicket(proceso)) {
        return {CodigoResultado::ARGUMENTO_INVALIDO, alta.pid};
    }
    memoriaUsadaBytes_ += alta.tamanioBytes;
    procesos_.push_back(std::move(proceso));
    registrar(TipoEvento::PROCESO_CREADO, alta.pid);
    registrar(TipoEvento::PROCESO_ADMITIDO, alta.pid);
    reclasificar();
    return {CodigoResultado::OK, alta.pid};
}

Resultado SimulationEngineV2::cancelarProceso(int pid) {
    Proceso* proceso = buscarProceso(pid);
    if (proceso == nullptr) {
        return {CodigoResultado::PID_INEXISTENTE, pid};
    }
    if (proceso->estado == Estado::FINISH) {
        return {CodigoResultado::PID_FINALIZADO, pid};
    }
    if (proceso->estado == Estado::CANCEL) {
        return {CodigoResultado::PID_YA_CANCELADO, pid};
    }

    // Un RUN no puede observarse externamente: siguientePaso es síncrono.
    proceso->estado = Estado::CANCEL;
    esperas_.erase(std::remove_if(esperas_.begin(), esperas_.end(),
                                  [pid](const Espera& espera) { return espera.pid == pid; }),
                   esperas_.end());
    liberarMemoria(*proceso);
    registrar(TipoEvento::PROCESO_CANCELADO, pid);
    reclasificar();
    finalizarSiCorresponde();
    return {CodigoResultado::OK, pid};
}

Resultado SimulationEngineV2::iniciar() {
    if (estado_ == EstadoEjecucion::FINALIZADO) {
        return {CodigoResultado::ESTADO_INVALIDO, -1};
    }
    estado_ = EstadoEjecucion::EJECUTANDO;
    return {CodigoResultado::OK, -1};
}

Resultado SimulationEngineV2::pausar() {
    if (estado_ != EstadoEjecucion::EJECUTANDO) {
        return {CodigoResultado::ESTADO_INVALIDO, -1};
    }
    estado_ = EstadoEjecucion::PAUSADO;
    return {CodigoResultado::OK, -1};
}

Resultado SimulationEngineV2::reiniciar() {
    reloj_.reiniciar();
    procesos_.clear();
    esperas_.clear();
    procesosConIOConsumida_.clear();
    eventos_.clear();
    timeline_.clear();
    estado_ = EstadoEjecucion::DETENIDO;
    memoriaUsadaBytes_ = 0;
    siguienteTicket_ = 1;
    pidEjecutando_ = -1;
    cuotaTurnoFijadaMs_ = 0;
    cpuConsumidaTurnoMs_ = 0;
    return {CodigoResultado::OK, -1};
}

ResultadoPaso SimulationEngineV2::siguientePaso() {
    if (estado_ != EstadoEjecucion::PAUSADO) {
        return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
    }

    reclasificar();
    const int pid = scheduler_.seleccionarSiguiente(procesos_);
    if (pid < 0) {
        if (esperas_.empty()) {
            finalizarSiCorresponde();
            return {CodigoResultado::OK, false, false, 0};
        }

        std::int64_t despertar = esperas_.front().despertarMs;
        for (const auto& espera : esperas_) {
            despertar = std::min(despertar, espera.despertarMs);
        }
        const std::int64_t inicio = reloj_.ahoraMs();
        if (despertar > inicio) {
            registrar(TipoEvento::CPU_IDLE_INICIO);
            reloj_.avanzar(despertar - inicio);
            registrar(TipoEvento::CPU_IDLE_FIN);
        }
        for (auto& espera : esperas_) {
            if (espera.despertarMs <= reloj_.ahoraMs()) {
                Proceso* proceso = buscarProceso(espera.pid);
                if (proceso != nullptr && proceso->estado == Estado::WAIT) {
                    proceso->estado = Estado::READY;
                    asignarTicket(*proceso);
                    registrar(TipoEvento::WAIT_A_READY, proceso->pid);
                }
            }
        }
        esperas_.erase(std::remove_if(esperas_.begin(), esperas_.end(),
                                      [this](const Espera& espera) {
                                          return espera.despertarMs <= reloj_.ahoraMs();
                                      }),
                       esperas_.end());
        reclasificar();
        return {CodigoResultado::OK, true, false, reloj_.ahoraMs() - inicio};
    }

    Proceso* proceso = buscarProceso(pid);
    proceso->estado = Estado::RUN;
    pidEjecutando_ = pid;
    cuotaTurnoFijadaMs_ = cuotaMs(proceso->prioridad);
    cpuConsumidaTurnoMs_ = 0;
    registrar(TipoEvento::READY_A_RUN, pid);

    std::int64_t duracion = std::min(cuotaTurnoFijadaMs_, proceso->tiempoCpuRestanteMs);
    const bool ioPendiente = proceso->operacionIO.habilitada &&
                             std::find(procesosConIOConsumida_.begin(),
                                       procesosConIOConsumida_.end(), pid) ==
                                 procesosConIOConsumida_.end();
    if (ioPendiente) {
        const std::int64_t hastaIO = proceso->operacionIO.despuesDeCpuMs - proceso->tiempoCpuConsumidoMs;
        duracion = std::min(duracion, hastaIO);
    }

    const std::int64_t inicio = reloj_.ahoraMs();
    reloj_.avanzar(duracion);
    proceso->tiempoCpuRestanteMs -= duracion;
    proceso->tiempoCpuConsumidoMs += duracion;
    cpuConsumidaTurnoMs_ = duracion;
    for (auto& otro : procesos_) {
        if (otro.estado == Estado::READY && otro.pid != pid) {
            otro.tiempoEsperaMs += duracion;
        }
    }
    timeline_.emplace_back(pid, inicio, reloj_.ahoraMs());
    registrar(TipoEvento::CPU_EJECUTADA, pid, duracion);

    if (proceso->tiempoCpuRestanteMs == 0) {
        proceso->estado = Estado::FINISH;
        liberarMemoria(*proceso);
        registrar(TipoEvento::RUN_A_FINISH, pid);
        reclasificar();
    } else if (ioPendiente && proceso->tiempoCpuConsumidoMs == proceso->operacionIO.despuesDeCpuMs) {
        proceso->estado = Estado::WAIT;
        esperas_.push_back({pid, reloj_.ahoraMs() + proceso->operacionIO.duracionEsperaMs});
        procesosConIOConsumida_.push_back(pid);
        registrar(TipoEvento::RUN_A_WAIT, pid);
        reclasificar();
    } else {
        proceso->estado = Estado::READY;
        asignarTicket(*proceso);
        registrar(TipoEvento::CUOTA_AGOTADA, pid);
        registrar(TipoEvento::RUN_A_READY, pid);
        reclasificar();
    }
    pidEjecutando_ = -1;
    cuotaTurnoFijadaMs_ = 0;
    cpuConsumidaTurnoMs_ = 0;
    finalizarSiCorresponde();
    return {CodigoResultado::OK, true, true, duracion};
}

EstadoSistema SimulationEngineV2::obtenerEstado() const {
    EstadoSistema estado;
    estado.tiempoSimuladoMs = reloj_.ahoraMs();
    estado.estadoEjecucion = estado_;
    estado.procesos = procesos_;
    estado.colaReady = scheduler_.ordenarColaPrioridad(procesos_);
    estado.pidEjecutando = pidEjecutando_;
    estado.cuotaTurnoFijadaMs = cuotaTurnoFijadaMs_;
    estado.cpuConsumidaTurnoMs = cpuConsumidaTurnoMs_;
    estado.memoriaUsadaBytes = memoriaUsadaBytes_;
    estado.memoriaTotalBytes = configuracion_.memoriaTotalBytes;
    estado.eventos = eventos_;
    estado.timeline = timeline_;
    return estado;
}

Proceso* SimulationEngineV2::buscarProceso(int pid) noexcept {
    for (auto& proceso : procesos_) {
        if (proceso.pid == pid) {
            return &proceso;
        }
    }
    return nullptr;
}

const Proceso* SimulationEngineV2::buscarProceso(int pid) const noexcept {
    for (const auto& proceso : procesos_) {
        if (proceso.pid == pid) {
            return &proceso;
        }
    }
    return nullptr;
}

void SimulationEngineV2::reclasificar() {
    std::vector<Prioridad> anteriores;
    anteriores.reserve(procesos_.size());
    for (const auto& proceso : procesos_) {
        anteriores.push_back(proceso.prioridad);
    }
    scheduler_.clasificarPrioridades(procesos_);
    for (std::size_t indice = 0; indice < procesos_.size(); ++indice) {
        if (procesos_[indice].prioridad != anteriores[indice]) {
            registrar(TipoEvento::PRIORIDAD_CAMBIADA, procesos_[indice].pid);
        }
    }
}

void SimulationEngineV2::registrar(TipoEvento tipo, int pid, std::int64_t duracionMs) {
    eventos_.push_back({reloj_.ahoraMs(), tipo, pid, duracionMs, {}});
}

bool SimulationEngineV2::asignarTicket(Proceso& proceso) noexcept {
    if (siguienteTicket_ == 0) {
        return false;
    }
    proceso.ticketReady = siguienteTicket_++;
    return true;
}

void SimulationEngineV2::liberarMemoria(const Proceso& proceso) noexcept {
    memoriaUsadaBytes_ -= proceso.tamanioBytes;
}

bool SimulationEngineV2::hayActivo() const noexcept {
    for (const auto& proceso : procesos_) {
        if (proceso.estado == Estado::READY || proceso.estado == Estado::RUN || proceso.estado == Estado::WAIT) {
            return true;
        }
    }
    return false;
}

void SimulationEngineV2::finalizarSiCorresponde() {
    if (!hayActivo() && estado_ != EstadoEjecucion::FINALIZADO) {
        estado_ = EstadoEjecucion::FINALIZADO;
        registrar(TipoEvento::SISTEMA_FINALIZADO);
    }
}

}  // namespace ConcurrentSimulator::v2
