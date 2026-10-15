#include "scheduler/SchedulerV2.h"
#include "tests/FixturesV2.h"
#include "tests/TestSupport.h"

#include <vector>

using namespace ConcurrentSimulator::v2;

int main() {
    Scheduler scheduler;

    // Caso 1: P3 tiene mayor prioridad que P2 y P1 (DP-01, DP-05)
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(1, "ProcP1", 100, Prioridad::P1, 10),
            crearProcesoV2(2, "ProcP3", 100, Prioridad::P3, 20),
            crearProcesoV2(3, "ProcP2", 100, Prioridad::P2, 30),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 2);
    }

    // Caso 2: P2 tiene mayor prioridad que P1
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(10, "ProcP1", 100, Prioridad::P1, 1),
            crearProcesoV2(20, "ProcP2", 100, Prioridad::P2, 5),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 20);
    }

    // Caso 3: Filtrar estrictamente por estado READY (ignorar WAIT, RUN, FINISH, CANCEL)
    // Aunque tengan P3, si no estan READY, gana el READY aunque tenga P1
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(1, "EnWait", 100, Prioridad::P3, 1, Estado::WAIT),
            crearProcesoV2(2, "EnRun", 100, Prioridad::P3, 2, Estado::RUN),
            crearProcesoV2(3, "EnFinish", 100, Prioridad::P3, 3, Estado::FINISH),
            crearProcesoV2(4, "EnCancel", 100, Prioridad::P3, 4, Estado::CANCEL),
            crearProcesoV2(5, "EnReady", 100, Prioridad::P1, 100, Estado::READY),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 5);
        std::vector<int> cola = scheduler.ordenarColaPrioridad(procesos);
        EXPECT_EQ(cola.size(), 1U);
        EXPECT_EQ(cola[0], 5);
    }

    // Caso 4: Ningun proceso en estado READY
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(1, "A", 100, Prioridad::P3, 1, Estado::WAIT),
            crearProcesoV2(2, "B", 100, Prioridad::P2, 2, Estado::FINISH),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), -1);
        EXPECT_TRUE(scheduler.ordenarColaPrioridad(procesos).empty());
    }

    // Caso 5: Vector vacio
    {
        std::vector<Proceso> procesos;
        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), -1);
        EXPECT_TRUE(scheduler.ordenarColaPrioridad(procesos).empty());
    }

    // Caso 6: Ordenar cola de prioridad y coherencia con seleccionarSiguiente
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(101, "P1", 100, Prioridad::P1, 1),
            crearProcesoV2(102, "P3", 100, Prioridad::P3, 2),
            crearProcesoV2(103, "P2", 100, Prioridad::P2, 3),
            crearProcesoV2(104, "P3_otro", 100, Prioridad::P3, 4),
        };

        std::vector<int> cola = scheduler.ordenarColaPrioridad(procesos);
        EXPECT_EQ(cola.size(), 4U);
        // Las dos P3 van primero (desempate por ticketReady: 2 antes que 4)
        EXPECT_EQ(cola[0], 102);
        EXPECT_EQ(cola[1], 104);
        EXPECT_EQ(cola[2], 103);
        EXPECT_EQ(cola[3], 101);

        // Coherencia absoluta: el ganador de seleccionarSiguiente es siempre cola[0]
        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), cola[0]);
    }

    return test::finish();
}
