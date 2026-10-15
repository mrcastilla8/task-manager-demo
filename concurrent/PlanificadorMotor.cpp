#include "concurrent/PlanificadorMotor.h"

int PlanificadorMotor::seleccionarSiguiente(
    const std::vector<Proceso>& procesos) const {
    return scheduler_.seleccionarSiguiente(procesos);
}

std::vector<int> PlanificadorMotor::ordenarCola(
    const std::vector<Proceso>& procesos) const {
    return scheduler_.ordenarColaPrioridad(procesos);
}

void PlanificadorMotor::alAsignarCPU(Proceso& proceso) const {
    scheduler_.alAsignarCPU(proceso);
}
