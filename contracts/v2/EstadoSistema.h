#pragma once

#include "contracts/v2/Estado.h"
#include "contracts/v2/Evento.h"
#include "contracts/v2/Proceso.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace ConcurrentSimulator::v2 {

struct BloqueTimeline {
    // Representa exclusivamente CPU realmente ejecutado por un proceso.
    // Los intervalos idle son huecos inferidos fuera de este tipo.
    BloqueTimeline(int pidProceso, std::int64_t inicio, std::int64_t fin)
        : pid_(pidProceso), inicioMs_(inicio), finMs_(fin) {
        if (pidProceso <= 0 || inicio < 0 || fin <= inicio) {
            throw std::invalid_argument("BloqueTimeline requiere PID positivo y un intervalo CPU no vacío");
        }
    }

    int pid() const noexcept { return pid_; }
    std::int64_t inicioMs() const noexcept { return inicioMs_; }
    std::int64_t finMs() const noexcept { return finMs_; }

private:
    int pid_;
    std::int64_t inicioMs_;
    std::int64_t finMs_;
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
    // Historial candidato; secuencia/orden para timestamps iguales está por
    // ratificar antes de depender de él para replay determinista.
    std::vector<Evento> eventos;
    // Intervalos CPU terminados, cronológicos, no superpuestos y con PID>0.
    // Idle se infiere como los huecos en [0, tiempoSimuladoMs).
    std::vector<BloqueTimeline> timeline;
};

}  // namespace ConcurrentSimulator::v2
