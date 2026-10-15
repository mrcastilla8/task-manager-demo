#include "scheduler/Scheduler.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

int main() {
    Scheduler scheduler;

    // Caso 1: Empate de prioridad resuelto por ordenLlegada (FIFO)
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 8, 1),
            crearProceso(2, "B", 100, Estado::READY, 8, 2),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 1);
        std::vector<int> orden = scheduler.ordenarColaPrioridad(procesos);
        EXPECT_EQ(orden.size(), 2U);
        EXPECT_EQ(orden[0], 1);
        EXPECT_EQ(orden[1], 2);
    }

    // Caso 2: Inversión en el orden del vector pero conservando ordenLlegada
    {
        std::vector<Proceso> procesos = {
            crearProceso(2, "B", 100, Estado::READY, 8, 2),
            crearProceso(1, "A", 100, Estado::READY, 8, 1),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 1);
        std::vector<int> orden = scheduler.ordenarColaPrioridad(procesos);
        EXPECT_EQ(orden.size(), 2U);
        EXPECT_EQ(orden[0], 1);
        EXPECT_EQ(orden[1], 2);
    }

    // Caso 3: Simulación Round Robin tras ejecución de quantum (DP-05)
    // El motor actualiza ordenLlegada de A a 3 al reingresar a READY tras agotar turno
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 8, 3),
            crearProceso(2, "B", 100, Estado::READY, 8, 2),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 2);
        std::vector<int> orden = scheduler.ordenarColaPrioridad(procesos);
        EXPECT_EQ(orden[0], 2);
        EXPECT_EQ(orden[1], 1);
    }

    // Caso 4: Empate en prioridad y ordenLlegada resuelto por PID menor
    {
        std::vector<Proceso> procesos = {
            crearProceso(5, "E", 100, Estado::READY, 8, 1),
            crearProceso(2, "B", 100, Estado::READY, 8, 1),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 2);
        std::vector<int> orden = scheduler.ordenarColaPrioridad(procesos);
        EXPECT_EQ(orden[0], 2);
        EXPECT_EQ(orden[1], 5);
    }

    // Caso 5: Desempate múltiple determinista (3 procesos con misma prioridad y llegada)
    {
        std::vector<Proceso> procesos = {
            crearProceso(9, "P9", 100, Estado::READY, 7, 10),
            crearProceso(3, "P3", 100, Estado::READY, 7, 10),
            crearProceso(5, "P5", 100, Estado::READY, 7, 10),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 3);

        std::vector<int> orden = scheduler.ordenarColaPrioridad(procesos);
        EXPECT_EQ(orden.size(), 3U);
        EXPECT_EQ(orden[0], 3);
        EXPECT_EQ(orden[1], 5);
        EXPECT_EQ(orden[2], 9);
    }

    return test::finish();
}
