#include "scheduler/SchedulerV2.h"
#include "tests/FixturesV2.h"
#include "tests/TestSupport.h"

#include <vector>

using namespace ConcurrentSimulator::v2;

int main() {
    Scheduler scheduler;

    // Caso 1: El tiempo de espera acumulado (tiempoEsperaMs) no altera la prioridad (DP-10)
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(1, "ProcViejo", 100, Prioridad::P1, 1, Estado::READY, 50'000), // Espero 50 segundos
            crearProcesoV2(2, "ProcNuevo", 100, Prioridad::P2, 2, Estado::READY, 0),      // Acaba de llegar pero es P2
        };

        // Gana el proceso P2 a pesar de que el proceso P1 acumulo gran tiempo de espera
        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 2);
    }

    // Caso 2: clasificarPrioridades no toma en cuenta tiempoEsperaMs, solo tamanioBytes
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(1, "A", 12, Prioridad::P3, 1, Estado::READY, 999'999),
            crearProcesoV2(2, "B", 96, Prioridad::P1, 2, Estado::READY, 0),
        };

        scheduler.clasificarPrioridades(procesos);

        // A tiene tamano 12 (tercio inferior) -> P1 independientemente de su espera
        EXPECT_EQ(procesos[0].prioridad, Prioridad::P1);
        // B tiene tamano 96 (tercio superior) -> P3 independientemente de que su espera sea 0
        EXPECT_EQ(procesos[1].prioridad, Prioridad::P3);
    }

    // Caso 3: Inanicion teorica de P1 ante flujo continuo de P2 o P3 (DP-10, Propuesta 8)
    // Procesos P1 permanecen en su prioridad y P2/P3 son seleccionados primero
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(10, "P1_esperando", 100, Prioridad::P1, 1, Estado::READY, 1'000'000),
            crearProcesoV2(20, "P2_activo", 100, Prioridad::P2, 100, Estado::READY, 10),
            crearProcesoV2(30, "P3_activo", 100, Prioridad::P3, 200, Estado::READY, 0),
        };

        std::vector<int> cola = scheduler.ordenarColaPrioridad(procesos);
        EXPECT_EQ(cola.size(), 3U);
        EXPECT_EQ(cola[0], 30); // P3
        EXPECT_EQ(cola[1], 20); // P2
        EXPECT_EQ(cola[2], 10); // P1 al final
    }

    return test::finish();
}
