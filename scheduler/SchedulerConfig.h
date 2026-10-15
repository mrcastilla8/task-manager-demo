#pragma once

#include "contracts/Configuracion.h"

#include <cstdint>

/**
 * @brief Configuración para el Scheduler.
 *
 * Centraliza los límites de prioridad e intervalos de aging según la política
 * acordada por el equipo, inicializándose con los contratos predeterminados.
 */
struct SchedulerConfig {
    int prioridadMinima{ConfiguracionPredeterminada::PRIORIDAD_MINIMA};
    int prioridadMaxima{ConfiguracionPredeterminada::PRIORIDAD_MAXIMA};
    std::int64_t agingIntervaloMs{ConfiguracionPredeterminada::AGING_INTERVALO_MS};
    int agingIncremento{ConfiguracionPredeterminada::AGING_INCREMENTO};

    constexpr bool esValida() const noexcept {
        return prioridadMinima >= 0 &&
               prioridadMaxima >= prioridadMinima &&
               agingIntervaloMs > 0 &&
               agingIncremento >= 0;
    }
};
