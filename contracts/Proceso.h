#pragma once

#include "contracts/Configuracion.h"
#include "contracts/Estado.h"

#include <cstdint>
#include <string>

struct OperacionIO {
    bool habilitada{false};
    std::int64_t despuesDeCpuMs{0};
    std::int64_t duracionEsperaMs{0};
};

struct Proceso {
    int pid{-1};
    std::string nombre;
    int tamanioBytes{0};

    int prioridadBase{1};
    int prioridadActual{1};

    std::int64_t tiempoTotal{0};
    std::int64_t tiempoRestante{0};
    std::int64_t tiempoEspera{0};

    std::int64_t quantumConsumido{0};

    Estado estado{Estado::READY};

    std::int64_t ordenLlegada{0};

    OperacionIO operacionIO;
};

inline bool prioridadValida(int prioridad) noexcept {
    return prioridad >= ConfiguracionPredeterminada::PRIORIDAD_MINIMA &&
           prioridad <= ConfiguracionPredeterminada::PRIORIDAD_MAXIMA;
}

inline bool procesoValido(const Proceso& proceso) noexcept {
    const bool camposBasicosValidos =
        proceso.pid > 0 && !proceso.nombre.empty() &&
        proceso.tamanioBytes > 0 &&
        prioridadValida(proceso.prioridadBase) &&
        prioridadValida(proceso.prioridadActual) &&
        proceso.tiempoTotal > 0 && proceso.tiempoRestante >= 0 &&
        proceso.tiempoRestante <= proceso.tiempoTotal &&
        proceso.tiempoEspera >= 0 && proceso.quantumConsumido >= 0 &&
        proceso.ordenLlegada >= 0;

    if (!camposBasicosValidos || !proceso.operacionIO.habilitada) {
        return camposBasicosValidos;
    }

    const OperacionIO& operacion = proceso.operacionIO;
    return operacion.despuesDeCpuMs > 0 &&
           operacion.despuesDeCpuMs < proceso.tiempoTotal &&
           operacion.duracionEsperaMs > 0;
}
