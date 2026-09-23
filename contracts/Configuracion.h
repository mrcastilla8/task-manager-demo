#pragma once

#include <cstdint>

namespace ConfiguracionPredeterminada {

constexpr int PRIORIDAD_MINIMA = 1;
constexpr int PRIORIDAD_MAXIMA = 10;
constexpr std::int64_t QUANTUM_MS = 500;
constexpr std::int64_t AGING_INTERVALO_MS = 2'000;
constexpr int AGING_INCREMENTO = 1;

}  // namespace ConfiguracionPredeterminada
