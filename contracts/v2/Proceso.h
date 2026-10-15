#pragma once

#include "contracts/v2/Estado.h"

#include <cstdint>
#include <string>

namespace ConcurrentSimulator::v2 {

enum class Prioridad : std::uint8_t { P1 = 1, P2 = 2, P3 = 3 };

struct OperacionIO {
    bool habilitada{false};
    std::int64_t despuesDeCpuMs{0};
    std::int64_t duracionEsperaMs{0};
};

// Solicitud de alta: el cliente no fija prioridad, cuota ni ticket.
struct AltaProceso {
    int pid{-1};
    std::string nombre;
    std::int64_t tamanioBytes{0};
    std::int64_t tiempoCpuTotalMs{0};
    OperacionIO operacionIO;
};

struct Proceso {
    int pid{-1};
    std::string nombre;
    std::int64_t tamanioBytes{0};
    Prioridad prioridad{Prioridad::P3};
    std::int64_t tiempoCpuTotalMs{0};
    std::int64_t tiempoCpuRestanteMs{0};
    std::int64_t tiempoCpuConsumidoMs{0};
    std::int64_t tiempoEsperaMs{0};
    std::uint64_t ticketReady{0};
    Estado estado{Estado::READY};
    OperacionIO operacionIO;
};

constexpr std::int64_t cuotaMs(Prioridad prioridad) noexcept {
    switch (prioridad) {
        case Prioridad::P1:
            return 10;
        case Prioridad::P2:
            return 20;
        case Prioridad::P3:
            return 30;
    }
    return 0;
}

constexpr std::int64_t cuotaVigenteMs(const Proceso& proceso) noexcept {
    return cuotaMs(proceso.prioridad);
}

}  // namespace ConcurrentSimulator::v2
