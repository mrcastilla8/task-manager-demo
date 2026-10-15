#pragma once

#include <cstdint>

namespace ConcurrentSimulator::v2 {

class SimulationClockV2 {
public:
    std::int64_t ahoraMs() const noexcept;
    void avanzar(std::int64_t duracionMs) noexcept;
    void reiniciar() noexcept;

private:
    std::int64_t tiempoMs_{0};
};

}  // namespace ConcurrentSimulator::v2
