#include "scheduler/Scheduler.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

int main() {
    Scheduler scheduler;

    // Caso 1: Aging incremental en pasos acumulados de espera
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 4),
        };

        EXPECT_EQ(procesos[0].prioridadActual, 4);
        EXPECT_EQ(procesos[0].tiempoEspera, 0);

        scheduler.aplicarAging(procesos, 1'000);
        EXPECT_EQ(procesos[0].prioridadActual, 4);
        EXPECT_EQ(procesos[0].tiempoEspera, 1'000);

        scheduler.aplicarAging(procesos, 1'000);
        EXPECT_EQ(procesos[0].prioridadActual, 5);
        EXPECT_EQ(procesos[0].tiempoEspera, 2'000);

        scheduler.aplicarAging(procesos, 2'000);
        EXPECT_EQ(procesos[0].prioridadActual, 6);
        EXPECT_EQ(procesos[0].tiempoEspera, 4'000);
    }

    // Caso 2: Salto con un solo delta grande (6,000 ms = 3 intervalos completos de 2,000 ms)
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 3),
        };

        scheduler.aplicarAging(procesos, 6'000);
        EXPECT_EQ(procesos[0].prioridadActual, 6);
        EXPECT_EQ(procesos[0].tiempoEspera, 6'000);
    }

    // Caso 3: Acumulación de deltas fraccionarios que cruzan el umbral
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 5),
        };

        scheduler.aplicarAging(procesos, 500);
        EXPECT_EQ(procesos[0].prioridadActual, 5);
        EXPECT_EQ(procesos[0].tiempoEspera, 500);

        scheduler.aplicarAging(procesos, 1'499);
        EXPECT_EQ(procesos[0].prioridadActual, 5);
        EXPECT_EQ(procesos[0].tiempoEspera, 1'999);

        // Al cruzar a 2,000 ms debe incrementar
        scheduler.aplicarAging(procesos, 1);
        EXPECT_EQ(procesos[0].prioridadActual, 6);
        EXPECT_EQ(procesos[0].tiempoEspera, 2'000);
    }

    // Caso 4: No supera el límite máximo de prioridad (10)
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 10),
        };

        scheduler.aplicarAging(procesos, 10'000);
        EXPECT_EQ(procesos[0].prioridadActual, 10);
        EXPECT_EQ(procesos[0].tiempoEspera, 10'000);
    }

    // Caso 5: Procesos que no están en READY no sufren aging
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::WAIT, 4),
            crearProceso(2, "B", 100, Estado::RUN, 4),
            crearProceso(3, "C", 100, Estado::FINISH, 4),
            crearProceso(4, "D", 100, Estado::CANCEL, 4),
        };

        scheduler.aplicarAging(procesos, 4'000);
        for (const auto& proc : procesos) {
            EXPECT_EQ(proc.prioridadActual, 4);
            EXPECT_EQ(proc.tiempoEspera, 0);
        }
    }

    // Caso 6: alAsignarCPU restablece la prioridad base y el tiempo de espera
    {
        Proceso proceso = crearProceso(1, "A", 100, Estado::READY, 4);
        std::vector<Proceso> vec = {proceso};

        scheduler.aplicarAging(vec, 4'000);
        EXPECT_EQ(vec[0].prioridadActual, 6);
        EXPECT_EQ(vec[0].tiempoEspera, 4'000);

        scheduler.alAsignarCPU(vec[0]);
        EXPECT_EQ(vec[0].prioridadActual, 4);
        EXPECT_EQ(vec[0].tiempoEspera, 0);
    }

    // Caso 7: Cambio dinámico de selección y reordenamiento de cola por aging (Caso P7)
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 5),
            crearProceso(2, "B", 100, Estado::READY, 4),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 1);

        // Tras 4,000 ms: A sube a 7, B sube a 6
        scheduler.aplicarAging(procesos, 4'000);
        EXPECT_EQ(procesos[0].prioridadActual, 7);
        EXPECT_EQ(procesos[1].prioridadActual, 6);

        // Si A es atendido, se le asigna CPU y se restablece
        scheduler.alAsignarCPU(procesos[0]);
        EXPECT_EQ(procesos[0].prioridadActual, 5);
        EXPECT_EQ(procesos[1].prioridadActual, 6);

        // Ahora B (prioridad 6) debe ser seleccionado sobre A (prioridad 5)
        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 2);

        // Comprobamos que el orden visible de la cola también refleja este cambio
        std::vector<int> orden = scheduler.ordenarColaPrioridad(procesos);
        EXPECT_EQ(orden.size(), 2U);
        EXPECT_EQ(orden[0], 2);
        EXPECT_EQ(orden[1], 1);
    }

    // Caso 8: Delta no positivo (<= 0) es ignorado de forma segura
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 5),
        };

        scheduler.aplicarAging(procesos, 0);
        EXPECT_EQ(procesos[0].prioridadActual, 5);
        EXPECT_EQ(procesos[0].tiempoEspera, 0);

        scheduler.aplicarAging(procesos, -500);
        EXPECT_EQ(procesos[0].prioridadActual, 5);
        EXPECT_EQ(procesos[0].tiempoEspera, 0);
    }

    return test::finish();
}
