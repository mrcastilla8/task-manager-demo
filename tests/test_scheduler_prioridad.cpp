#include "scheduler/Scheduler.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

int main() {
    Scheduler scheduler;

    // Caso 1: Seleccionar mayor prioridad actual (A=8, B=5, C=3)
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 8),
            crearProceso(2, "B", 100, Estado::READY, 5),
            crearProceso(3, "C", 100, Estado::READY, 3),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 1);
    }

    // Caso 2: Caso de referencia P5 (CASOS_REFERENCIA.md)
    {
        std::vector<Proceso> procesos = {
            crearProceso(10, "ProcA", 200, Estado::READY, 8),
            crearProceso(20, "ProcB", 300, Estado::READY, 5),
            crearProceso(30, "ProcC", 150, Estado::READY, 3),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 10);
    }

    // Caso 3: Filtrar por estado READY (ignorar WAIT, RUN, FINISH, CANCEL con prioridad mayor)
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::WAIT, 10),
            crearProceso(2, "B", 100, Estado::READY, 5),
            crearProceso(3, "C", 100, Estado::RUN, 9),
            crearProceso(4, "D", 100, Estado::FINISH, 10),
            crearProceso(5, "E", 100, Estado::CANCEL, 10),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 2);
    }

    // Caso 4: Ningún proceso en estado READY
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::WAIT, 10),
            crearProceso(2, "B", 100, Estado::FINISH, 8),
            crearProceso(3, "C", 100, Estado::CANCEL, 7),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), -1);
        EXPECT_TRUE(scheduler.ordenarColaPrioridad(procesos).empty());
    }

    // Caso 5: Vector vacío
    {
        std::vector<Proceso> procesos;
        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), -1);
        EXPECT_TRUE(scheduler.ordenarColaPrioridad(procesos).empty());
    }

    // Caso 6: Ordenar cola de prioridad y coherencia con seleccionarSiguiente
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 3),
            crearProceso(2, "B", 100, Estado::READY, 8),
            crearProceso(3, "C", 100, Estado::WAIT, 10),
            crearProceso(4, "D", 100, Estado::READY, 5),
        };

        std::vector<int> orden = scheduler.ordenarColaPrioridad(procesos);
        EXPECT_EQ(orden.size(), 3U);
        EXPECT_EQ(orden[0], 2);
        EXPECT_EQ(orden[1], 4);
        EXPECT_EQ(orden[2], 1);

        // Coherencia: el ganador de seleccionarSiguiente es siempre orden[0]
        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), orden[0]);
    }

    return test::finish();
}
