#pragma once

#include <cstdint>
#include <string>

enum class TipoEvento {
    PROCESS_CREATED,
    PROCESS_ADMITTED,
    PROCESS_READY,
    PROCESS_RUNNING,
    QUANTUM_FINISHED,
    PROCESS_WAITING,
    PROCESS_FINISHED,
    PROCESS_CANCELLED,
    PRIORITY_CHANGED
};

inline std::string tipoEventoToString(TipoEvento tipo) {
    switch (tipo) {
        case TipoEvento::PROCESS_CREATED:
            return "PROCESS_CREATED";
        case TipoEvento::PROCESS_ADMITTED:
            return "PROCESS_ADMITTED";
        case TipoEvento::PROCESS_READY:
            return "PROCESS_READY";
        case TipoEvento::PROCESS_RUNNING:
            return "PROCESS_RUNNING";
        case TipoEvento::QUANTUM_FINISHED:
            return "QUANTUM_FINISHED";
        case TipoEvento::PROCESS_WAITING:
            return "PROCESS_WAITING";
        case TipoEvento::PROCESS_FINISHED:
            return "PROCESS_FINISHED";
        case TipoEvento::PROCESS_CANCELLED:
            return "PROCESS_CANCELLED";
        case TipoEvento::PRIORITY_CHANGED:
            return "PRIORITY_CHANGED";
    }

    return "UNKNOWN";
}

struct Evento {
    std::int64_t tiempoSimulado{0};
    TipoEvento tipo{TipoEvento::PROCESS_CREATED};
    int pid{-1};
    std::string descripcion;
};
