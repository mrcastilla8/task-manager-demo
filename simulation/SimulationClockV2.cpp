#include "simulation/SimulationClockV2.h"

namespace ConcurrentSimulator::v2 {

std::int64_t SimulationClockV2::ahoraMs() const noexcept {
    return tiempoMs_;
}

void SimulationClockV2::avanzar(std::int64_t duracionMs) noexcept {
    tiempoMs_ += duracionMs;
}

void SimulationClockV2::reiniciar() noexcept {
    tiempoMs_ = 0;
}

}  // namespace ConcurrentSimulator::v2
