#pragma once

#include <cstdint>
#include <string>

enum class MotivoFinTurno {
    NINGUNO,
    QUANTUM_AGOTADO,
    PROCESO_TERMINADO,
    BLOQUEO_IO,
    CANCELADO
};

inline std::string motivoFinTurnoToString(MotivoFinTurno motivo) {
    switch (motivo) {
        case MotivoFinTurno::NINGUNO:
            return "NINGUNO";
        case MotivoFinTurno::QUANTUM_AGOTADO:
            return "QUANTUM_AGOTADO";
        case MotivoFinTurno::PROCESO_TERMINADO:
            return "PROCESO_TERMINADO";
        case MotivoFinTurno::BLOQUEO_IO:
            return "BLOQUEO_IO";
        case MotivoFinTurno::CANCELADO:
            return "CANCELADO";
    }
    return "UNKNOWN";
}

struct SolicitudTurno {
    std::int64_t cuotaMs{0};
    std::int64_t tiempoRestante{0};
    bool tieneIO{false};
    std::int64_t cpuRestanteParaIO{0};
    // Solo pruebas: duerme fisicamente dentro del worker, de forma interrumpible,
    // para ensanchar la ventana en que la rafaga esta en curso. No altera el
    // tiempo simulado.
    std::int64_t retardoRealMs{0};
};

struct ResultadoRafaga {
    int pid{-1};
    std::int64_t tiempoEjecutadoMs{0};
    MotivoFinTurno motivo{MotivoFinTurno::NINGUNO};
};
