#pragma once

#include "contracts/Configuracion.h"

#include <cstdint>

struct SchedulerConfig {
    int prioridadMinima{ConfiguracionPredeterminada::PRIORIDAD_MINIMA};
    int prioridadMaxima{ConfiguracionPredeterminada::PRIORIDAD_MAXIMA};
    std::int64_t agingIntervaloMs{ConfiguracionPredeterminada::AGING_INTERVALO_MS};
    int agingIncremento{ConfiguracionPredeterminada::AGING_INCREMENTO};
};
