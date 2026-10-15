#pragma once

#include <cstdint>
#include <string>

namespace ConcurrentSimulator::v2 {

enum class TipoEvento {
    PROCESO_CREADO,
    PROCESO_ADMITIDO,
    PRIORIDAD_CAMBIADA,
    READY_A_RUN,
    CPU_EJECUTADA,
    CUOTA_AGOTADA,
    RUN_A_READY,
    RUN_A_WAIT,
    WAIT_A_READY,
    RUN_A_FINISH,
    PROCESO_CANCELADO,
    CPU_IDLE_INICIO,
    CPU_IDLE_FIN,
    SISTEMA_FINALIZADO
};

struct Evento {
    std::int64_t tiempoSimuladoMs{0};
    TipoEvento tipo{TipoEvento::PROCESO_CREADO};
    int pid{-1};
    std::int64_t duracionMs{0};
    std::string detalle;
};

}  // namespace ConcurrentSimulator::v2
