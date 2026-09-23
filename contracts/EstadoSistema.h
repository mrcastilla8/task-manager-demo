#pragma once

#include "contracts/Configuracion.h"
#include "contracts/Evento.h"
#include "contracts/Proceso.h"

#include <cstdint>
#include <string>
#include <vector>

struct BloqueTimeline {
    int pid{-1};
    std::int64_t inicio{0};
    std::int64_t fin{0};
};

inline bool bloqueTimelineValido(const BloqueTimeline& bloque) noexcept {
    return bloque.pid > 0 && bloque.inicio >= 0 && bloque.fin > bloque.inicio;
}

enum class EstadoEjecucion {
    DETENIDO,
    EJECUTANDO,
    PAUSADO,
    FINALIZADO
};

inline std::string estadoEjecucionToString(EstadoEjecucion estado) {
    switch (estado) {
        case EstadoEjecucion::DETENIDO:
            return "DETENIDO";
        case EstadoEjecucion::EJECUTANDO:
            return "EJECUTANDO";
        case EstadoEjecucion::PAUSADO:
            return "PAUSADO";
        case EstadoEjecucion::FINALIZADO:
            return "FINALIZADO";
    }

    return "UNKNOWN";
}

struct EstadoSistema {
    std::int64_t tiempoSimulado{0};
    EstadoEjecucion estadoEjecucion{EstadoEjecucion::DETENIDO};
    std::int64_t quantumMs{ConfiguracionPredeterminada::QUANTUM_MS};

    std::vector<Proceso> procesos;
    std::vector<int> colaPrioridad;

    int pidEjecutando{-1};

    int memoriaUsada{0};
    int memoriaTotal{0};

    std::vector<Evento> eventos;
    std::vector<BloqueTimeline> timeline;
};
