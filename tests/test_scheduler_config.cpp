#include "scheduler/Scheduler.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

int main() {
    // Caso 1: Configuración predeterminada
    {
        Scheduler scheduler;
        const auto& cfg = scheduler.configuracion();

        EXPECT_EQ(cfg.prioridadMinima, ConfiguracionPredeterminada::PRIORIDAD_MINIMA);
        EXPECT_EQ(cfg.prioridadMaxima, ConfiguracionPredeterminada::PRIORIDAD_MAXIMA);
        EXPECT_EQ(cfg.agingIntervaloMs, ConfiguracionPredeterminada::AGING_INTERVALO_MS);
        EXPECT_EQ(cfg.agingIncremento, ConfiguracionPredeterminada::AGING_INCREMENTO);
        EXPECT_TRUE(cfg.esValida());
    }

    // Caso 2: Modificación dinámica con configurar()
    {
        Scheduler scheduler;

        SchedulerConfig nuevaCfg;
        nuevaCfg.prioridadMinima = 1;
        nuevaCfg.prioridadMaxima = 20;
        nuevaCfg.agingIntervaloMs = 500;
        nuevaCfg.agingIncremento = 2;

        EXPECT_TRUE(nuevaCfg.esValida());
        scheduler.configurar(nuevaCfg);

        EXPECT_EQ(scheduler.configuracion().prioridadMaxima, 20);
        EXPECT_EQ(scheduler.configuracion().agingIntervaloMs, 500);
        EXPECT_EQ(scheduler.configuracion().agingIncremento, 2);

        // Proceso supera la antigua cota de 10 con la nueva configuración
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 10),
        };

        // Con intervalo de 500 ms y delta de 1,000 ms, son 2 intervalos * 2 = +4 incremento
        scheduler.aplicarAging(procesos, 1'000);
        EXPECT_EQ(procesos[0].prioridadActual, 14);
        EXPECT_EQ(procesos[0].tiempoEspera, 1'000);
    }

    // Caso 3: Aging incremento cero (desactiva aging efectivamente)
    {
        SchedulerConfig cfgSinAging;
        cfgSinAging.agingIncremento = 0;
        EXPECT_TRUE(cfgSinAging.esValida());

        Scheduler scheduler(cfgSinAging);
        std::vector<Proceso> procesos = {
            crearProceso(1, "A", 100, Estado::READY, 5),
        };

        scheduler.aplicarAging(procesos, 5'000);
        EXPECT_EQ(procesos[0].prioridadActual, 5);
        // El tiempo de espera no se altera si incremento <= 0
        EXPECT_EQ(procesos[0].tiempoEspera, 0);
    }

    // Caso 4: Invariantes de esValida()
    {
        SchedulerConfig invalida1{-1, 10, 2000, 1}; // min negativo
        EXPECT_FALSE(invalida1.esValida());

        SchedulerConfig invalida2{10, 5, 2000, 1};  // max < min
        EXPECT_FALSE(invalida2.esValida());

        SchedulerConfig invalida3{1, 10, 0, 1};     // intervalo <= 0
        EXPECT_FALSE(invalida3.esValida());

        SchedulerConfig invalida4{1, 10, 2000, -1}; // incremento < 0
        EXPECT_FALSE(invalida4.esValida());
    }

    return test::finish();
}
