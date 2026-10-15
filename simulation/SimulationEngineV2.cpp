#include "simulation/SimulationEngineV2.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace ConcurrentSimulator::v2 {

SimulationEngineV2::SimulationEngineV2(Configuracion configuracion)
    : configuracion_(configuracion), memoria_(configuracion.memoriaTotalBytes) {}

Resultado SimulationEngineV2::agregarProceso(const AltaProceso& alta) {
    const ResultadoAdmision admision =
        admitirProceso(alta, procesos_, memoria_);
    switch (admision) {
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

    try {
        if (!asignarTicket(alta.pid)) {
            (void)procesos_.eliminar(alta.pid);
            liberarMemoria(alta.pid);
            return {CodigoResultado::ESTADO_INVALIDO, alta.pid};
        }
    } catch (const std::overflow_error&) {
        (void)procesos_.eliminar(alta.pid);
        liberarMemoria(alta.pid);
        return {CodigoResultado::ESTADO_INVALIDO, alta.pid};
    }

    if (estado_ == EstadoEjecucion::FINALIZADO) {
        estado_ = EstadoEjecucion::DETENIDO;
    }
    registrar(TipoEvento::PROCESO_CREADO, alta.pid);
    registrar(TipoEvento::PROCESO_ADMITIDO, alta.pid);
    reclasificar();
    return {CodigoResultado::OK, alta.pid};
}

Resultado SimulationEngineV2::cancelarProceso(int pid) {
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

    // El motor lógico completa cada ráfaga de forma síncrona. RUN no puede
    // observarse desde otra llamada; la cancelación parcial pertenece al
    // protocolo concurrente G1 y no se aproxima con tiempo físico.
    if (!procesos_.actualizarEstado(pid, Estado::CANCEL)) {
        return {CodigoResultado::ESTADO_INVALIDO, pid};
    }
    esperas_.erase(std::remove_if(esperas_.begin(), esperas_.end(),
                                  [pid](const Espera& espera) {
                                      return espera.pid == pid;
                                  }),
                   esperas_.end());
    procesosConIOConsumida_.erase(
        std::remove(procesosConIOConsumida_.begin(),
                    procesosConIOConsumida_.end(), pid),
        procesosConIOConsumida_.end());
    liberarMemoria(pid);
    registrar(TipoEvento::PROCESO_CANCELADO, pid);
    reclasificar();
    finalizarSiCorresponde();
    return {CodigoResultado::OK, pid};
}

Resultado SimulationEngineV2::iniciar() {
    if (estado_ == EstadoEjecucion::FINALIZADO) {
        return {CodigoResultado::ESTADO_INVALIDO, -1};
    }
    // La ejecución continua requiere un driver; iniciar no bloquea ni avanza
    // el reloj mientras G1 no ratifique su transporte.
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
    procesos_ = ListaProcesos{};
    memoria_ = Memoria(configuracion_.memoriaTotalBytes);
    esperas_.clear();
    procesosConIOConsumida_.clear();
    eventos_.clear();
    timeline_.clear();
    estado_ = EstadoEjecucion::DETENIDO;
    asignadorTickets_.reiniciar();
    pidEjecutando_ = -1;
    cuotaTurnoFijadaMs_ = 0;
    cpuConsumidaTurnoMs_ = 0;
    return {CodigoResultado::OK, -1};
}

ResultadoPaso SimulationEngineV2::siguientePaso() {
    if (estado_ != EstadoEjecucion::PAUSADO) {
        return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
    }

    try {
        if (!despertarEsperasVencidas()) {
            return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
        }
    } catch (const std::overflow_error&) {
        return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
    }

    const std::vector<Proceso> vistaReady = procesos_.obtenerTodos();
    const int pid = scheduler_.seleccionarSiguiente(vistaReady);
    if (pid < 0) {
        if (esperas_.empty()) {
            finalizarSiCorresponde();
            return {CodigoResultado::OK, false, false, 0};
        }

        std::int64_t despertar = esperas_.front().despertarMs;
        for (const auto& espera : esperas_) {
            despertar = (std::min)(despertar, espera.despertarMs);
        }
        const std::int64_t inicio = reloj_.ahoraMs();
        if (despertar > inicio) {
            const std::int64_t delta = despertar - inicio;
            if (!reloj_.puedeAvanzar(delta)) {
                return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
            }
            registrar(TipoEvento::CPU_IDLE_INICIO);
            reloj_.avanzar(delta);
            registrar(TipoEvento::CPU_IDLE_FIN);
        }
        try {
            if (!despertarEsperasVencidas()) {
                return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
            }
        } catch (const std::overflow_error&) {
            return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
        }
        return {CodigoResultado::OK, true, false, reloj_.ahoraMs() - inicio};
    }

    const Proceso* seleccionado = procesos_.buscar(pid);
    if (seleccionado == nullptr || seleccionado->estado != Estado::READY) {
        return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
    }
    const std::int64_t cuota = cuotaMs(seleccionado->prioridad);
    std::int64_t duracion = (std::min)(cuota, seleccionado->tiempoCpuRestanteMs);
    const bool ioPendiente = seleccionado->operacionIO.habilitada &&
        std::find(procesosConIOConsumida_.begin(),
                  procesosConIOConsumida_.end(), pid) ==
            procesosConIOConsumida_.end();
    if (ioPendiente) {
        const std::int64_t hastaIO =
            seleccionado->operacionIO.despuesDeCpuMs -
            seleccionado->tiempoCpuConsumidoMs;
        duracion = (std::min)(duracion, hastaIO);
    }

    const std::int64_t inicio = reloj_.ahoraMs();
    if (duracion <= 0 || !reloj_.puedeAvanzar(duracion) ||
        duracion > (std::numeric_limits<std::int64_t>::max)() - inicio) {
        return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
    }
    const std::int64_t finMaximo = inicio + duracion;

    // Preflight tickets para todos los WAIT que pueden despertar durante este
    // paso y para el proceso que reingresará por cuota. Así el agotamiento no
    // deja un turno a medias ni renumera tickets sin ratificación.
    std::size_t ticketsNecesarios = 0;
    for (const Espera& espera : esperas_) {
        if (espera.despertarMs <= finMaximo) {
            const Proceso* esperando = procesos_.buscar(espera.pid);
            if (esperando != nullptr && esperando->estado == Estado::WAIT) {
                ++ticketsNecesarios;
            }
        }
    }
    const bool alcanzaCPU = duracion == seleccionado->tiempoCpuRestanteMs;
    const bool alcanzaIO = ioPendiente && duracion ==
        seleccionado->operacionIO.despuesDeCpuMs -
            seleccionado->tiempoCpuConsumidoMs;
    const bool vuelveReady = !alcanzaCPU && !alcanzaIO;
    if (vuelveReady) {
        ++ticketsNecesarios;
    }
    const std::uint64_t proximo = asignadorTickets_.proximo();
    const std::uint64_t disponibles =
        (std::numeric_limits<std::uint64_t>::max)() - proximo;
    if (ticketsNecesarios > disponibles) {
        return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
    }

    if (!procesos_.actualizarEstado(pid, Estado::RUN)) {
        return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
    }
    pidEjecutando_ = pid;
    cuotaTurnoFijadaMs_ = cuota;
    cpuConsumidaTurnoMs_ = 0;
    registrar(TipoEvento::READY_A_RUN, pid);

    std::int64_t pendiente = duracion;
    while (pendiente > 0) {
        std::int64_t segmento = pendiente;
        for (const Espera& espera : esperas_) {
            if (espera.despertarMs > reloj_.ahoraMs()) {
                segmento = (std::min)(segmento,
                                      espera.despertarMs - reloj_.ahoraMs());
            }
        }
        if (segmento <= 0 || !reloj_.puedeAvanzar(segmento)) {
            return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
        }

        const Proceso* actual = procesos_.buscar(pid);
        if (actual == nullptr ||
            !procesos_.actualizarCpu(pid,
                                     actual->tiempoCpuConsumidoMs + segmento,
                                     actual->tiempoCpuRestanteMs - segmento)) {
            return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
        }
        reloj_.avanzar(segmento);
        cpuConsumidaTurnoMs_ += segmento;
        pendiente -= segmento;

        for (const Proceso& otro : procesos_.obtenerTodos()) {
            if (otro.estado == Estado::READY && otro.pid != pid) {
                const auto maximo =
                    (std::numeric_limits<std::int64_t>::max)();
                const std::int64_t espera =
                    otro.tiempoEsperaMs > maximo - segmento
                        ? maximo
                        : otro.tiempoEsperaMs + segmento;
                (void)procesos_.actualizarTiempoEspera(otro.pid, espera);
            }
        }
        registrar(TipoEvento::CPU_EJECUTADA, pid, segmento);
        try {
            if (!despertarEsperasVencidas()) {
                return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
            }
        } catch (const std::overflow_error&) {
            return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
        }
    }

    timeline_.emplace_back(pid, inicio, reloj_.ahoraMs());
    const Proceso* proceso = procesos_.buscar(pid);
    if (proceso == nullptr) {
        return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
    }
    if (proceso->tiempoCpuRestanteMs == 0) {
        if (!procesos_.actualizarEstado(pid, Estado::FINISH)) {
            return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
        }
        liberarMemoria(pid);
        registrar(TipoEvento::RUN_A_FINISH, pid);
        reclasificar();
    } else if (ioPendiente &&
               proceso->tiempoCpuConsumidoMs ==
                   proceso->operacionIO.despuesDeCpuMs) {
        esperas_.push_back({pid, reloj_.ahoraMs() +
                                    proceso->operacionIO.duracionEsperaMs});
        procesosConIOConsumida_.push_back(pid);
        if (!procesos_.actualizarEstado(pid, Estado::WAIT)) {
            esperas_.pop_back();
            procesosConIOConsumida_.pop_back();
            return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
        }
        registrar(TipoEvento::RUN_A_WAIT, pid);
    } else {
        if (!asignarTicket(pid) ||
            !procesos_.actualizarEstado(pid, Estado::READY)) {
            return {CodigoResultado::ESTADO_INVALIDO, false, false, 0};
        }
        registrar(TipoEvento::CUOTA_AGOTADA, pid);
        registrar(TipoEvento::RUN_A_READY, pid);
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

void SimulationEngineV2::reclasificar() {
    std::vector<Proceso> vista = procesos_.obtenerTodos();
    std::vector<Prioridad> anteriores;
    anteriores.reserve(vista.size());
    for (const Proceso& proceso : vista) {
        anteriores.push_back(proceso.prioridad);
    }
    scheduler_.clasificarPrioridades(vista);
    if (!procesos_.aplicarPrioridades(vista)) {
        throw std::logic_error(
            "La vista de procesos cambió antes de persistir prioridades");
    }
    for (std::size_t indice = 0; indice < vista.size(); ++indice) {
        if (vista[indice].prioridad != anteriores[indice]) {
            registrar(TipoEvento::PRIORIDAD_CAMBIADA, vista[indice].pid);
        }
    }
}

void SimulationEngineV2::registrar(TipoEvento tipo, int pid,
                                   std::int64_t duracionMs) {
    eventos_.push_back({reloj_.ahoraMs(), tipo, pid, duracionMs, {}});
}

bool SimulationEngineV2::asignarTicket(int pid) {
    const std::uint64_t ticket = asignadorTickets_.asignar();
    return procesos_.actualizarTicketReady(pid, ticket);
}

bool SimulationEngineV2::despertarEsperasVencidas() {
    std::vector<Espera> vencidas;
    for (const Espera& espera : esperas_) {
        const Proceso* proceso = procesos_.buscar(espera.pid);
        if (espera.despertarMs <= reloj_.ahoraMs() && proceso != nullptr &&
            proceso->estado == Estado::WAIT) {
            vencidas.push_back(espera);
        }
    }
    const std::uint64_t proximo = asignadorTickets_.proximo();
    const std::uint64_t disponibles =
        (std::numeric_limits<std::uint64_t>::max)() - proximo;
    if (vencidas.size() > disponibles) {
        throw std::overflow_error("Se agotó el espacio de tickets READY");
    }

    std::vector<std::uint64_t> nuevosTickets;
    nuevosTickets.reserve(vencidas.size());
    for (std::size_t indice = 0; indice < vencidas.size(); ++indice) {
        nuevosTickets.push_back(asignadorTickets_.asignar());
    }
    for (std::size_t indice = 0; indice < vencidas.size(); ++indice) {
        const int pid = vencidas[indice].pid;
        if (!procesos_.actualizarTicketReady(pid, nuevosTickets[indice]) ||
            !procesos_.actualizarEstado(pid, Estado::READY)) {
            throw std::logic_error("No se pudo despertar un proceso WAIT");
        }
        registrar(TipoEvento::WAIT_A_READY, pid);
    }
    esperas_.erase(std::remove_if(esperas_.begin(), esperas_.end(),
                                  [this](const Espera& espera) {
                                      return espera.despertarMs <=
                                             reloj_.ahoraMs();
                                  }),
                   esperas_.end());
    return true;
}

void SimulationEngineV2::liberarMemoria(int pid) noexcept {
    (void)memoria_.liberar(pid);
}

bool SimulationEngineV2::hayActivo() const {
    for (const Proceso& proceso : procesos_.obtenerTodos()) {
        if (proceso.estado == Estado::READY || proceso.estado == Estado::RUN ||
            proceso.estado == Estado::WAIT) {
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
