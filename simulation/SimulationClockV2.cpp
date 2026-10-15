#include "simulation/SimulationClockV2.h"

#include <limits>

namespace ConcurrentSimulator::v2 {

std::int64_t SimulationClockV2::ahoraMs() const noexcept {
    return tiempoMs_;
}

bool SimulationClockV2::puedeAvanzar(std::int64_t duracionMs) const noexcept {
    return duracionMs >= 0 &&
           duracionMs <= std::numeric_limits<std::int64_t>::max() - tiempoMs_;
}

void SimulationClockV2::avanzar(std::int64_t duracionMs) noexcept {
    if (duracionMs > 0 && puedeAvanzar(duracionMs)) {
        tiempoMs_ += duracionMs;
    }
}

void SimulationClockV2::reiniciar() noexcept {
    tiempoMs_ = 0;
}

}  // namespace ConcurrentSimulator::v2
