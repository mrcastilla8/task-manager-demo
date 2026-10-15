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
    // Solo pruebas: el worker espera `ProcessThread::liberarRafaga()` antes de
    // calcular la rafaga, con un limite maximo para no colgar nunca. No es un
    // retardo fisico de CPU y no interviene en el reloj ni en el CPU consumido.
    bool esperarLiberacion{false};
    std::int64_t limiteEsperaMs{5000};
};

struct ResultadoRafaga {
    int pid{-1};
    std::int64_t tiempoEjecutadoMs{0};
    MotivoFinTurno motivo{MotivoFinTurno::NINGUNO};
};
