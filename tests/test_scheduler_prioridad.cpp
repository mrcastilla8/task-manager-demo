#include "scheduler/Scheduler.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

int main() {
    Scheduler scheduler;

    // Caso: Seleccionar mayor prioridad actual
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 8),
            crearProceso(2, "B", 100, Estado::READY, 5),
            crearProceso(3, "C", 100, Estado::READY, 3),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 1);
    }

    // Caso: Filtrar por estado READY (ignorar WAIT, RUN, FINISH, CANCEL)
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

    // Caso: Ningún proceso en estado READY
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::WAIT, 10),
            crearProceso(2, "B", 100, Estado::FINISH, 8),
            crearProceso(3, "C", 100, Estado::CANCEL, 7),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), -1);
    }

    // Caso: Vector vacío
    {
        std::vector<Proceso> procesos;
        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), -1);
    }

    // Caso: Ordenar cola de prioridad
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
    }

    return test::finish();
}
