#pragma once

#include <cstdint>

namespace ConcurrentSimulator::v2 {

// Candidata a adopción. La política funcional está fijada; la capacidad
// disponible pertenece a la configuración de cada simulador.
struct Configuracion {
    std::int64_t memoriaTotalBytes{0};
};

}  // namespace ConcurrentSimulator::v2
