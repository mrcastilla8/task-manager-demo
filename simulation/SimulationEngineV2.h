#pragma once

#include "contracts/v2/IExecutionEngine.h"
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

    Proceso* buscarProceso(int pid) noexcept;
    const Proceso* buscarProceso(int pid) const noexcept;
    void reclasificar();
    void registrar(TipoEvento tipo, int pid = -1, std::int64_t duracionMs = 0);
    bool asignarTicket(Proceso& proceso);
    bool despertarEsperasVencidas();
    void liberarMemoria(const Proceso& proceso) noexcept;
    bool hayActivo() const noexcept;
    void finalizarSiCorresponde();

    Configuracion configuracion_;
    SimulationClockV2 reloj_;
    Scheduler scheduler_;
    std::vector<Proceso> procesos_;
    std::vector<Espera> esperas_;
    std::vector<int> procesosConIOConsumida_;
    std::vector<Evento> eventos_;
    std::vector<BloqueTimeline> timeline_;
    EstadoEjecucion estado_{EstadoEjecucion::DETENIDO};
    std::int64_t memoriaUsadaBytes_{0};
    std::uint64_t siguienteTicket_{1};
    int pidEjecutando_{-1};
    std::int64_t cuotaTurnoFijadaMs_{0};
    std::int64_t cpuConsumidaTurnoMs_{0};
};

}  // namespace ConcurrentSimulator::v2
