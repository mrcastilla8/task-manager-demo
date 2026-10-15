#include "simulation/SimulationEngineV2.h"

#include <algorithm>
#include <limits>
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
    if (estado_ == EstadoEjecucion::FINALIZADO) {
        estado_ = EstadoEjecucion::DETENIDO;
    }
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

    // Un RUN no puede cancelarse por la API: siguientePaso es síncrono y solo
    // retorna tras transitarlo a READY, WAIT o FINISH; no hay hilos en este motor.
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
    // La ejecucion continua corresponde al driver; esta operacion no despacha ni bloquea.
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

    if (!despertarEsperasVencidas()) {
        return {CodigoResultado::ARGUMENTO_INVALIDO, false, false, 0};
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
            if (!reloj_.puedeAvanzar(despertar - inicio)) {
                return {CodigoResultado::ARGUMENTO_INVALIDO, false, false, 0};
            }
            registrar(TipoEvento::CPU_IDLE_INICIO);
            reloj_.avanzar(despertar - inicio);
            registrar(TipoEvento::CPU_IDLE_FIN);
        }
        if (!despertarEsperasVencidas()) {
            return {CodigoResultado::ARGUMENTO_INVALIDO, false, false, 0};
        }
        reclasificar();
        return {CodigoResultado::OK, true, false, reloj_.ahoraMs() - inicio};
    }

    Proceso* proceso = buscarProceso(pid);
    const std::int64_t cuota = cuotaMs(proceso->prioridad);
    std::int64_t duracion = std::min(cuota, proceso->tiempoCpuRestanteMs);
    const bool ioPendiente = proceso->operacionIO.habilitada &&
                             std::find(procesosConIOConsumida_.begin(),
                                       procesosConIOConsumida_.end(), pid) ==
                                 procesosConIOConsumida_.end();
    if (ioPendiente) {
        const std::int64_t hastaIO = proceso->operacionIO.despuesDeCpuMs - proceso->tiempoCpuConsumidoMs;
        duracion = std::min(duracion, hastaIO);
    }

    const std::int64_t inicio = reloj_.ahoraMs();
    if (duracion <= 0 || !reloj_.puedeAvanzar(duracion) ||
        (ioPendiente && duracion == proceso->operacionIO.despuesDeCpuMs -
                                         proceso->tiempoCpuConsumidoMs &&
         proceso->operacionIO.duracionEsperaMs >
             std::numeric_limits<std::int64_t>::max() - (inicio + duracion))) {
        return {CodigoResultado::ARGUMENTO_INVALIDO, false, false, 0};
    }

    proceso->estado = Estado::RUN;
    pidEjecutando_ = pid;
    cuotaTurnoFijadaMs_ = cuota;
    cpuConsumidaTurnoMs_ = 0;
    registrar(TipoEvento::READY_A_RUN, pid);

    std::int64_t pendiente = duracion;
    while (pendiente > 0) {
        std::int64_t segmento = pendiente;
        for (const auto& espera : esperas_) {
            if (espera.despertarMs > reloj_.ahoraMs()) {
                segmento = std::min(segmento, espera.despertarMs - reloj_.ahoraMs());
            }
        }

        if (segmento <= 0 || !reloj_.puedeAvanzar(segmento)) {
            return {CodigoResultado::ARGUMENTO_INVALIDO, false, false, 0};
        }
        reloj_.avanzar(segmento);
        proceso->tiempoCpuRestanteMs -= segmento;
        proceso->tiempoCpuConsumidoMs += segmento;
        cpuConsumidaTurnoMs_ += segmento;
        pendiente -= segmento;
        for (auto& otro : procesos_) {
            if (otro.estado == Estado::READY && otro.pid != pid) {
                otro.tiempoEsperaMs += segmento;
            }
        }
        registrar(TipoEvento::CPU_EJECUTADA, pid, segmento);
        if (!despertarEsperasVencidas()) {
            return {CodigoResultado::ARGUMENTO_INVALIDO, false, false, 0};
        }
    }
    timeline_.emplace_back(pid, inicio, reloj_.ahoraMs());

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
        if (!asignarTicket(*proceso)) {
            return {CodigoResultado::ARGUMENTO_INVALIDO, false, false, 0};
        }
        proceso->estado = Estado::READY;
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

bool SimulationEngineV2::asignarTicket(Proceso& proceso) {
    if (siguienteTicket_ == 0 ||
        siguienteTicket_ == std::numeric_limits<std::uint64_t>::max()) {
        std::vector<Proceso*> listos;
        listos.reserve(procesos_.size());
        for (auto& candidato : procesos_) {
            if (candidato.estado == Estado::READY) {
                listos.push_back(&candidato);
            }
        }
        std::sort(listos.begin(), listos.end(), [](const Proceso* a, const Proceso* b) {
            return a->ticketReady != b->ticketReady ? a->ticketReady < b->ticketReady
                                                     : a->pid < b->pid;
        });
        for (std::size_t indice = 0; indice < listos.size(); ++indice) {
            if (indice >= std::numeric_limits<std::uint64_t>::max() - 1) {
                return false;
            }
            listos[indice]->ticketReady = static_cast<std::uint64_t>(indice) + 1;
        }
        siguienteTicket_ = static_cast<std::uint64_t>(listos.size()) + 1;
    }
    if (siguienteTicket_ == 0 ||
        siguienteTicket_ == std::numeric_limits<std::uint64_t>::max()) {
        return false;
    }
    proceso.ticketReady = siguienteTicket_;
    ++siguienteTicket_;
    return true;
}

bool SimulationEngineV2::despertarEsperasVencidas() {
    for (auto& espera : esperas_) {
        if (espera.despertarMs > reloj_.ahoraMs()) {
            continue;
        }
        Proceso* proceso = buscarProceso(espera.pid);
        if (proceso != nullptr && proceso->estado == Estado::WAIT) {
            if (!asignarTicket(*proceso)) {
                return false;
            }
            proceso->estado = Estado::READY;
            registrar(TipoEvento::WAIT_A_READY, proceso->pid);
        }
    }
    esperas_.erase(std::remove_if(esperas_.begin(), esperas_.end(),
                                  [this](const Espera& espera) {
                                      return espera.despertarMs <= reloj_.ahoraMs();
                                  }),
                   esperas_.end());
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
