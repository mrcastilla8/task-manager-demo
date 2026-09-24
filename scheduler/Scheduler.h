#pragma once

#include "contracts/Proceso.h"
#include "scheduler/SchedulerConfig.h"

#include <cstdint>
#include <vector>

class Scheduler {
public:
    explicit Scheduler(SchedulerConfig config = {}) noexcept;

    const SchedulerConfig& configuracion() const noexcept;
    void configurar(const SchedulerConfig& config) noexcept;

    int seleccionarSiguiente(const std::vector<Proceso>& procesos) const;

    void aplicarAging(std::vector<Proceso>& procesos,
                      std::int64_t deltaTiempo) const;

    void alAsignarCPU(Proceso& proceso) const noexcept;

    std::vector<int> ordenarColaPrioridad(
        const std::vector<Proceso>& procesos) const;

private:
    SchedulerConfig config_;
};
