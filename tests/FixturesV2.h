#pragma once

#include "contracts/v2/Estado.h"
#include "contracts/v2/Proceso.h"

#include <cstdint>
#include <string>
#include <utility>

namespace ConcurrentSimulator::v2 {

inline Proceso crearProcesoV2(int pid,
                              std::string nombre,
                              std::int64_t tamanioBytes,
                              Prioridad prioridad = Prioridad::P3,
                              std::uint64_t ticketReady = 0,
                              Estado estado = Estado::READY,
                              std::int64_t tiempoEsperaMs = 0) {
    return Proceso{
        pid,
        std::move(nombre),
        tamanioBytes,
        prioridad,
        2'000,
        2'000,
        0,
        tiempoEsperaMs,
        ticketReady,
        estado,
        OperacionIO{}
    };
}

}  // namespace ConcurrentSimulator::v2
