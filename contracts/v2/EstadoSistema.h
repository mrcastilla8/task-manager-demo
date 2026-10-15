#pragma once

#include "contracts/v2/Estado.h"
#include "contracts/v2/Evento.h"
#include "contracts/v2/Proceso.h"

#include <cstdint>
#include <vector>

namespace ConcurrentSimulator::v2 {

struct BloqueTimeline {
    // pid == 0 representa CPU idle; los PID de proceso deben ser positivos.
    int pid{0};
    std::int64_t inicioMs{0};
    std::int64_t finMs{0};
};

struct EstadoSistema {
    std::int64_t tiempoSimuladoMs{0};
    EstadoEjecucion estadoEjecucion{EstadoEjecucion::DETENIDO};
    std::vector<Proceso> procesos;  // Incluye FINISH/CANCEL durante la sesión.
    std::vector<int> colaReady;     // Orden efectivo del siguiente despacho.
    int pidEjecutando{-1};
    std::int64_t cuotaTurnoFijadaMs{0};
    std::int64_t cpuConsumidaTurnoMs{0};
    std::int64_t memoriaUsadaBytes{0};
    std::int64_t memoriaTotalBytes{0};
    std::vector<Evento> eventos;
    std::vector<BloqueTimeline> timeline;
};

}  // namespace ConcurrentSimulator::v2
