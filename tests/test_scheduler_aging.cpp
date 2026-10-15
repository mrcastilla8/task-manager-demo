#include "scheduler/Scheduler.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

int main() {
    Scheduler scheduler;

    // Caso: Aging incrementa prioridad tras 2000 ms de espera
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

    // Caso: No supera el límite máximo de prioridad (10)
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 10),
        };

        scheduler.aplicarAging(procesos, 10'000);
        EXPECT_EQ(procesos[0].prioridadActual, 10);
    }

    // Caso: Procesos que no están en READY no sufren aging
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::WAIT, 4),
            crearProceso(2, "B", 100, Estado::RUN, 4),
        };

        scheduler.aplicarAging(procesos, 4'000);
        EXPECT_EQ(procesos[0].prioridadActual, 4);
        EXPECT_EQ(procesos[0].tiempoEspera, 0);
        EXPECT_EQ(procesos[1].prioridadActual, 4);
        EXPECT_EQ(procesos[1].tiempoEspera, 0);
    }

    // Caso: alAsignarCPU restablece la prioridad base y el tiempo de espera
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

    // Caso: Selección cambia dinámicamente gracias al aging
    {
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 5),
            crearProceso(2, "B", 100, Estado::READY, 4),
        };

        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 1);

        // B espera 4000 ms -> sube a prioridad 6
        scheduler.aplicarAging(procesos, 4'000);
        EXPECT_EQ(procesos[0].prioridadActual, 7); // A sube de 5 a 7
        EXPECT_EQ(procesos[1].prioridadActual, 6); // B sube de 4 a 6

        // Si A se atiende y se resetea:
        scheduler.alAsignarCPU(procesos[0]);
        // Ahora A tiene prioridadActual 5 y B tiene 6
        EXPECT_EQ(scheduler.seleccionarSiguiente(procesos), 2);
    }

    return test::finish();
}
