#include "scheduler/Scheduler.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

int main() {
    Scheduler scheduler;

    // Caso: Empate de prioridad resuelto por ordenLlegada (FIFO)
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 8, 1),
            crearProceso(2, "B", 100, Estado::READY, 8, 2),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 1);
    }

    // Caso: Inversión en el orden del vector pero conservando ordenLlegada
    {
        std::vector<Proceso> procesos = {
            crearProceso(2, "B", 100, Estado::READY, 8, 2),
            crearProceso(1, "A", 100, Estado::READY, 8, 1),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 1);
    }

    // Caso: Simulación Round Robin tras ejecución de quantum
    // Proceso A consume quantum y su ordenLlegada se actualiza a 3 al volver a READY
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 8, 3),
            crearProceso(2, "B", 100, Estado::READY, 8, 2),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 2);
    }

    // Caso: Empate en prioridad y ordenLlegada resuelto por PID menor
    {
        std::vector<Proceso> procesos = {
            crearProceso(5, "E", 100, Estado::READY, 8, 1),
            crearProceso(2, "B", 100, Estado::READY, 8, 1),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 2);
    }

    return test::finish();
}
