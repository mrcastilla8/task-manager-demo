#pragma once

#include "contracts/Proceso.h"
#include "scheduler/Scheduler.h"

#include <cstdint>
#include <vector>

// Unico punto del modulo concurrente que conoce el Scheduler compartido
// (responsabilidad de Brickzon). El motor solo habla con esta clase: cuando se
// migre al Scheduler v2 (prioridades P1-P3, tickets, sin aging) basta con
// cambiar este adaptador, sin tocar `scheduler/` ni la logica de los workers.
//
// No contiene politica propia: delega en el Scheduler sin reordenar ni filtrar.
class PlanificadorMotor {
public:
    PlanificadorMotor() = default;

    // PID del proceso READY que recibe el CPU logico, o -1 si no hay ninguno.
    int seleccionarSiguiente(const std::vector<Proceso>& procesos) const;

    // PID de los procesos READY en el orden en que se representan.
    std::vector<int> ordenarCola(const std::vector<Proceso>& procesos) const;

    // Preparacion de un proceso al recibir el CPU.
    void alAsignarCPU(Proceso& proceso) const;

    // v1 heredado: DP-10 retira el aging en v2.
    void aplicarAging(std::vector<Proceso>& procesos, std::int64_t delta) const;

private:
    Scheduler scheduler_;
};
