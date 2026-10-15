#include "gui/Presentation.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace gui {
const char* stateName(api::Estado value) {
    switch (value) {
    case api::Estado::READY: return "READY";
    case api::Estado::RUN: return "RUN";
    case api::Estado::WAIT: return "WAIT";
    case api::Estado::FINISH: return "FINISH";
    case api::Estado::CANCEL: return "CANCEL";
    }
    return "?";
}
const char* executionName(api::EstadoEjecucion value) {
    switch (value) {
    case api::EstadoEjecucion::DETENIDO: return "Detenido";
    case api::EstadoEjecucion::EJECUTANDO: return "Ejecutando";
    case api::EstadoEjecucion::PAUSADO: return "Pausado";
    case api::EstadoEjecucion::FINALIZADO: return "Finalizado";
    }
    return "?";
}
const char* eventName(api::TipoEvento value) {
    switch (value) {
    case api::TipoEvento::PROCESO_CREADO: return "Creado";
    case api::TipoEvento::PROCESO_ADMITIDO: return "Admitido";
    case api::TipoEvento::PRIORIDAD_CAMBIADA: return "Prioridad publicada";
    case api::TipoEvento::READY_A_RUN: return "Despacho";
    case api::TipoEvento::CPU_EJECUTADA: return "CPU ejecutada";
    case api::TipoEvento::CUOTA_AGOTADA: return "Cuota agotada";
    case api::TipoEvento::RUN_A_READY: return "RUN → READY";
    case api::TipoEvento::RUN_A_WAIT: return "RUN → WAIT";
    case api::TipoEvento::WAIT_A_READY: return "WAIT → READY";
    case api::TipoEvento::RUN_A_FINISH: return "Finalización";
    case api::TipoEvento::PROCESO_CANCELADO: return "Cancelación";
    case api::TipoEvento::CPU_IDLE_INICIO: return "Inicio idle";
    case api::TipoEvento::CPU_IDLE_FIN: return "Fin idle";
    case api::TipoEvento::SISTEMA_FINALIZADO: return "Sistema finalizado";
    }
    return "?";
}
const char* resultName(api::CodigoResultado value) {
    switch (value) {
    case api::CodigoResultado::OK: return "Operación completada";
    case api::CodigoResultado::ARGUMENTO_INVALIDO: return "Datos inválidos";
    case api::CodigoResultado::PID_DUPLICADO: return "El PID ya existe";
    case api::CodigoResultado::MEMORIA_INSUFICIENTE: return "Memoria insuficiente";
    case api::CodigoResultado::PID_INEXISTENTE: return "El PID no existe";
    case api::CodigoResultado::PID_FINALIZADO: return "El proceso ya finalizó";
    case api::CodigoResultado::PID_YA_CANCELADO: return "El proceso ya fue cancelado";
    case api::CodigoResultado::ESTADO_INVALIDO: return "Acción no disponible en este estado";
    }
    return "Resultado desconocido";
}
bool active(api::Estado value) {
    return value == api::Estado::READY || value == api::Estado::RUN || value == api::Estado::WAIT;
}
const api::Proceso* findProcess(const api::EstadoSistema& state, int pid) {
    auto it = std::find_if(state.procesos.begin(), state.procesos.end(),
                           [pid](const auto& process) { return process.pid == pid; });
    return it == state.procesos.end() ? nullptr : &*it;
}
std::vector<TimelineSpan> timelineSpans(const api::EstadoSistema& state) {
    std::vector<TimelineSpan> result;
    std::int64_t cursor = 0;
    for (const auto& block : state.timeline) {
        if (block.inicioMs() < cursor || block.finMs() > state.tiempoSimuladoMs)
            throw std::invalid_argument("Timeline inconsistente con el snapshot");
        if (block.inicioMs() > cursor) result.push_back({-1, cursor, block.inicioMs()});
        result.push_back({block.pid(), block.inicioMs(), block.finMs()});
        cursor = block.finMs();
    }
    if (state.tiempoSimuladoMs < 0) throw std::invalid_argument("Reloj negativo");
    if (cursor < state.tiempoSimuladoMs) result.push_back({-1, cursor, state.tiempoSimuladoMs});
    return result;
}
std::string validateInput(const api::AltaProceso& input) {
    if (input.pid <= 0) return "El PID debe ser positivo.";
    if (input.nombre.empty() || std::all_of(input.nombre.begin(), input.nombre.end(),
        [](unsigned char c) { return std::isspace(c) != 0; })) return "Escribe un nombre.";
    if (input.tamanioBytes <= 0) return "El tamaño debe ser positivo (bytes).";
    if (input.tiempoCpuTotalMs <= 0) return "La CPU total debe ser positiva (ms).";
    const auto& io = input.operacionIO;
    if (io.habilitada && (io.despuesDeCpuMs <= 0 || io.despuesDeCpuMs >= input.tiempoCpuTotalMs ||
                         io.duracionEsperaMs <= 0))
        return "E/S requiere un umbral entre 0 y CPU total, y espera positiva.";
    return {};
}
} // namespace gui
