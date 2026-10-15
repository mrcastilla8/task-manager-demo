#include "scheduler/SchedulerV2.h"
#include "tests/FixturesV2.h"
#include "tests/TestSupport.h"

#include <vector>

using namespace ConcurrentSimulator::v2;

int main() {
    Scheduler scheduler;

    // Caso 1: Empate en prioridad resuelto por menor ticketReady (FIFO)
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(1, "A", 100, Prioridad::P2, 10),
            crearProcesoV2(2, "B", 100, Prioridad::P2, 20),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 1);
        std::vector<int> cola = scheduler.ordenarColaPrioridad(procesos);
        EXPECT_EQ(cola.size(), 2U);
        EXPECT_EQ(cola[0], 1);
        EXPECT_EQ(cola[1], 2);
    }

    // Caso 2: Inversion en el orden fisico del vector conserva determinismo por ticketReady
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(2, "B", 100, Prioridad::P2, 20),
            crearProcesoV2(1, "A", 100, Prioridad::P2, 10),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 1);
        std::vector<int> cola = scheduler.ordenarColaPrioridad(procesos);
        EXPECT_EQ(cola[0], 1);
        EXPECT_EQ(cola[1], 2);
    }

    // Caso 3: Simulacion de alternancia Round Robin tras agotar turno (DP-05)
    // Inicialmente A (ticket 10) y B (ticket 20). Gana A.
    // Tras su turno, A reingresa a READY con nuevo ticket asignado estrictamente creciente (ticket 30).
    {
        std::vector<Proceso> estadoPaso1 = {
            crearProcesoV2(1, "A", 100, Prioridad::P3, 10),
            crearProcesoV2(2, "B", 100, Prioridad::P3, 20),
        };
        EXPECT_EQ(scheduler.seleccionarSiguiente(estadoPaso1), 1);

        // Simulacion del motor: A agoto su cuota y reingreso a READY con ticket 30
        std::vector<Proceso> estadoPaso2 = {
            crearProcesoV2(1, "A", 100, Prioridad::P3, 30),
            crearProcesoV2(2, "B", 100, Prioridad::P3, 20),
        };
        EXPECT_EQ(scheduler.seleccionarSiguiente(estadoPaso2), 2);

        // Ahora B agota su cuota y reingresa con ticket 40
        std::vector<Proceso> estadoPaso3 = {
            crearProcesoV2(1, "A", 100, Prioridad::P3, 30),
            crearProcesoV2(2, "B", 100, Prioridad::P3, 40),
        };
        EXPECT_EQ(scheduler.seleccionarSiguiente(estadoPaso3), 1);
    }

    // Caso 4: Empate simultaneo en prioridad y ticketReady resuelto por menor PID
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(50, "P50", 100, Prioridad::P1, 100),
            crearProcesoV2(20, "P20", 100, Prioridad::P1, 100),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 20);
        std::vector<int> cola = scheduler.ordenarColaPrioridad(procesos);
        EXPECT_EQ(cola[0], 20);
        EXPECT_EQ(cola[1], 50);
    }

    // Caso 5: Desempate multiple (3 procesos con misma prioridad y ticket)
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(90, "P90", 100, Prioridad::P2, 5),
            crearProcesoV2(15, "P15", 100, Prioridad::P2, 5),
            crearProcesoV2(45, "P45", 100, Prioridad::P2, 5),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 15);
        std::vector<int> cola = scheduler.ordenarColaPrioridad(procesos);
        EXPECT_EQ(cola.size(), 3U);
        EXPECT_EQ(cola[0], 15);
        EXPECT_EQ(cola[1], 45);
        EXPECT_EQ(cola[2], 90);
    }

    return test::finish();
}
