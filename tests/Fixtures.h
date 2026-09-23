#pragma once

#include "contracts/Proceso.h"

#include <cstdint>
#include <string>
#include <utility>

inline Proceso crearProceso(int pid, std::string nombre, int tamanio,
                            Estado estado = Estado::READY,
                            int prioridad = 5,
                            std::int64_t ordenLlegada = 0) {
    return Proceso{pid,
                   std::move(nombre),
                   tamanio,
                   prioridad,
                   prioridad,
                   2'000,
                   2'000,
                   0,
                   0,
                   estado,
                   ordenLlegada,
                   OperacionIO{}};
}
