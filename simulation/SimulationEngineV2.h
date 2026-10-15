#pragma once

#include "contracts/v2/IExecutionEngine.h"
#include "core/v2/AdmisorProcesos.h"
#include "core/v2/ListaProcesos.h"
#include "core/v2/Memoria.h"
#include "scheduler/SchedulerV2.h"
#include "simulation/SimulationClockV2.h"

#include <cstdint>
#include <vector>

namespace ConcurrentSimulator::v2 {

class SimulationEngineV2 final : public IExecutionEngine {
public:
    explicit SimulationEngineV2(Configuracion configuracion);

    Resultado agregarProceso(const AltaProceso& proceso) override;
    Resultado cancelarProceso(int pid) override;
    Resultado iniciar() override;
    Resultado pausar() override;
    Resultado reiniciar() override;
    ResultadoPaso siguientePaso() override;
    EstadoSistema obtenerEstado() const override;

private:
    struct Espera {
        int pid{-1};
        std::int64_t despertarMs{0};
    };

    void reclasificar();
    void registrar(TipoEvento tipo, int pid = -1,
                   std::int64_t duracionMs = 0);
    bool asignarTicket(int pid);
    bool despertarEsperasVencidas();
    void liberarMemoria(int pid) noexcept;
    bool hayActivo() const;
    void finalizarSiCorresponde();

    Configuracion configuracion_;
    SimulationClockV2 reloj_;
    ListaProcesos procesos_;
    Memoria memoria_;
    Scheduler scheduler_;
    AsignadorTickets asignadorTickets_;
    std::vector<Espera> esperas_;
    std::vector<int> procesosConIOConsumida_;
    std::vector<Evento> eventos_;
    std::vector<BloqueTimeline> timeline_;
    EstadoEjecucion estado_{EstadoEjecucion::DETENIDO};
    int pidEjecutando_{-1};
    std::int64_t cuotaTurnoFijadaMs_{0};
    std::int64_t cpuConsumidaTurnoMs_{0};
};

}  // namespace ConcurrentSimulator::v2
