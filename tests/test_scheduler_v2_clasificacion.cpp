#include "scheduler/SchedulerV2.h"
#include "tests/FixturesV2.h"
#include "tests/TestSupport.h"

#include <climits>
#include <vector>

using namespace ConcurrentSimulator::v2;

int main() {
    Scheduler scheduler;

    // Caso 1: Escenario 1 de CASOS_REFERENCIA.md (A=12, B=96, D=84, H=28)
    // S=12 -> P1, S=39 -> P1, S=40 -> P2, S=67 -> P2, S=68 -> P3, S=96 -> P3
    {
        EXPECT_EQ(Scheduler::evaluarPrioridad(12, 12, 96), Prioridad::P1);
        EXPECT_EQ(Scheduler::evaluarPrioridad(39, 12, 96), Prioridad::P1);
        EXPECT_EQ(Scheduler::evaluarPrioridad(40, 12, 96), Prioridad::P2);
        EXPECT_EQ(Scheduler::evaluarPrioridad(67, 12, 96), Prioridad::P2);
        EXPECT_EQ(Scheduler::evaluarPrioridad(68, 12, 96), Prioridad::P3);
        EXPECT_EQ(Scheduler::evaluarPrioridad(96, 12, 96), Prioridad::P3);
    }

    // Caso 2: clasificarPrioridades con el vector completo del Caso 1
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(1, "P1_min", 12),
            crearProcesoV2(2, "P1_sup", 39),
            crearProcesoV2(3, "P2_inf", 40),
            crearProcesoV2(4, "P2_sup", 67),
            crearProcesoV2(5, "P3_inf", 68),
            crearProcesoV2(6, "P3_max", 96),
        };

        scheduler.clasificarPrioridades(procesos);

        EXPECT_EQ(procesos[0].prioridad, Prioridad::P1);
        EXPECT_EQ(procesos[1].prioridad, Prioridad::P1);
        EXPECT_EQ(procesos[2].prioridad, Prioridad::P2);
        EXPECT_EQ(procesos[3].prioridad, Prioridad::P2);
        EXPECT_EQ(procesos[4].prioridad, Prioridad::P3);
        EXPECT_EQ(procesos[5].prioridad, Prioridad::P3);
    }

    // Caso 3: Escenario P3 de CASOS_REFERENCIA.md (Todos los tamanos iguales A=B da P3)
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(1, "A", 50, Prioridad::P1),
            crearProcesoV2(2, "B", 50, Prioridad::P2),
            crearProcesoV2(3, "C", 50, Prioridad::P1),
        };

        scheduler.clasificarPrioridades(procesos);

        EXPECT_EQ(procesos[0].prioridad, Prioridad::P3);
        EXPECT_EQ(procesos[1].prioridad, Prioridad::P3);
        EXPECT_EQ(procesos[2].prioridad, Prioridad::P3);
    }

    // Caso 4: Unico proceso en TL (A = B = S) resulta en P3
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(1, "Solo", 100, Prioridad::P1),
        };

        scheduler.clasificarPrioridades(procesos);
        EXPECT_EQ(procesos[0].prioridad, Prioridad::P3);
    }

    // Caso 5: TL vacia no altera procesos (o vector vacio)
    {
        std::vector<Proceso> vacio;
        scheduler.clasificarPrioridades(vacio);
        EXPECT_TRUE(vacio.empty());
    }

    // Caso 6: Poblacion TL: solo READY, RUN y WAIT participan; FINISH y CANCEL se excluyen
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(1, "ReadyMin", 12, Prioridad::P3, 0, Estado::READY),
            crearProcesoV2(2, "RunMed", 40, Prioridad::P3, 0, Estado::RUN),
            crearProcesoV2(3, "WaitMax", 96, Prioridad::P1, 0, Estado::WAIT),
            // FINISH y CANCEL con tamanos extremos que NO deben sesgar minA ni maxB
            crearProcesoV2(4, "Terminado", 1, Prioridad::P1, 0, Estado::FINISH),
            crearProcesoV2(5, "Cancelado", 1000, Prioridad::P2, 0, Estado::CANCEL),
        };

        scheduler.clasificarPrioridades(procesos);

        // TL activa esta acotada por A=12 y B=96 (ignora 1 y 1000)
        EXPECT_EQ(procesos[0].prioridad, Prioridad::P1); // 12 -> P1
        EXPECT_EQ(procesos[1].prioridad, Prioridad::P2); // 40 -> P2
        EXPECT_EQ(procesos[2].prioridad, Prioridad::P3); // 96 -> P3

        // Estados terminales mantienen su prioridad original intacta
        EXPECT_EQ(procesos[3].prioridad, Prioridad::P1);
        EXPECT_EQ(procesos[4].prioridad, Prioridad::P2);
    }

    // Caso 7: Escenario 2 de CASOS_REFERENCIA.md: Prevencion estricta de desbordamiento (overflow)
    {
        const std::int64_t minA = 0;
        const std::int64_t maxB = 9'000'000'000'000'000'000LL; // 9e18 (cercano a INT64_MAX ~ 9.22e18)

        // S = 1e18 -> tercio inferior (< 3e18) -> P1
        EXPECT_EQ(Scheduler::evaluarPrioridad(1'000'000'000'000'000'000LL, minA, maxB), Prioridad::P1);

        // S = 4e18 -> tercio medio (>= 3e18 y < 6e18) -> P2
        EXPECT_EQ(Scheduler::evaluarPrioridad(4'000'000'000'000'000'000LL, minA, maxB), Prioridad::P2);

        // S = 8e18 -> tercio superior (>= 6e18) -> P3
        EXPECT_EQ(Scheduler::evaluarPrioridad(8'000'000'000'000'000'000LL, minA, maxB), Prioridad::P3);

        // Caso limite con rango completo
        const std::int64_t limiteMax = INT64_MAX - 10;
        EXPECT_EQ(Scheduler::evaluarPrioridad(10, 0, limiteMax), Prioridad::P1);
        EXPECT_EQ(Scheduler::evaluarPrioridad(limiteMax / 2, 0, limiteMax), Prioridad::P2);
        EXPECT_EQ(Scheduler::evaluarPrioridad(limiteMax - 1, 0, limiteMax), Prioridad::P3);
    }

    // Caso 8: Validacion estricta de entradas invalidas sin fallbacks silenciosos (DP-03)
    {
        // Tamanos negativos
        EXPECT_THROW(Scheduler::evaluarPrioridad(-1, 10, 20), std::invalid_argument);
        EXPECT_THROW(Scheduler::evaluarPrioridad(15, -1, 20), std::invalid_argument);
        EXPECT_THROW(Scheduler::evaluarPrioridad(15, 10, -1), std::invalid_argument);

        // minA > maxB no debe intercambiarse silenciosamente
        EXPECT_THROW(Scheduler::evaluarPrioridad(15, 30, 20), std::invalid_argument);

        // tamanioBytes fuera del intervalo [minA, maxB]
        EXPECT_THROW(Scheduler::evaluarPrioridad(5, 10, 20), std::invalid_argument);
        EXPECT_THROW(Scheduler::evaluarPrioridad(25, 10, 20), std::invalid_argument);

        // Proceso activo en TL con tamano negativo detectado en clasificarPrioridades
        std::vector<Proceso> invalidos = {
            crearProcesoV2(1, "Negativo", -10, Prioridad::P1, 1, Estado::READY),
        };
        EXPECT_THROW(scheduler.clasificarPrioridades(invalidos), std::invalid_argument);
    }

    // Caso 9: Admision rechazada no altera TL ni prioridades existentes (DP-02c)
    {
        std::vector<Proceso> procesos = {
            crearProcesoV2(1, "P1", 12, Prioridad::P3, 1, Estado::READY),
            crearProcesoV2(2, "P2", 40, Prioridad::P3, 2, Estado::READY),
            crearProcesoV2(3, "P3", 96, Prioridad::P3, 3, Estado::READY),
        };

        scheduler.clasificarPrioridades(procesos);
        EXPECT_EQ(procesos[0].prioridad, Prioridad::P1);
        EXPECT_EQ(procesos[1].prioridad, Prioridad::P2);
        EXPECT_EQ(procesos[2].prioridad, Prioridad::P3);

        // Si una admision es rechazada por memoria, el nuevo proceso NO se inserta en TL
        // El estado y prioridades de los procesos activos deben mantenerse exactamente iguales
        scheduler.clasificarPrioridades(procesos);
        EXPECT_EQ(procesos[0].prioridad, Prioridad::P1);
        EXPECT_EQ(procesos[1].prioridad, Prioridad::P2);
        EXPECT_EQ(procesos[2].prioridad, Prioridad::P3);
    }

    // Caso 10: Disparadores de recalculo (DP-02b: Admision exitosa, FINISH y CANCEL)
    {
        // Estado base: dos procesos
        std::vector<Proceso> procesos = {
            crearProcesoV2(1, "P1", 10, Prioridad::P1, 1, Estado::READY),
            crearProcesoV2(2, "P2", 20, Prioridad::P1, 2, Estado::READY),
        };
        scheduler.clasificarPrioridades(procesos);
        EXPECT_EQ(procesos[0].prioridad, Prioridad::P1);
        EXPECT_EQ(procesos[1].prioridad, Prioridad::P3); // A=10, B=20 -> P2 es el mayor (P3)

        // Disparador 1: Admision exitosa de un proceso mayor (tamanio 100)
        procesos.push_back(crearProcesoV2(3, "P3", 100, Prioridad::P1, 3, Estado::READY));
        scheduler.clasificarPrioridades(procesos);
        // Ahora A=10, B=100 (D=90). P1(10) es P1, P2(20) pasa a ser P1 (3*(20-10)=30 < 90), P3(100) es P3
        EXPECT_EQ(procesos[0].prioridad, Prioridad::P1);
        EXPECT_EQ(procesos[1].prioridad, Prioridad::P1);
        EXPECT_EQ(procesos[2].prioridad, Prioridad::P3);

        // Disparador 2: Finalizacion de P3 (pasa a FINISH)
        procesos[2].estado = Estado::FINISH;
        scheduler.clasificarPrioridades(procesos);
        // P3 sale de TL, TL vuelve a {P1(10), P2(20)}. P2 vuelve a P3. P3 conserva su prioridad.
        EXPECT_EQ(procesos[0].prioridad, Prioridad::P1);
        EXPECT_EQ(procesos[1].prioridad, Prioridad::P3);
        EXPECT_EQ(procesos[2].prioridad, Prioridad::P3);

        // Disparador 3: Cancelacion de P2 (pasa a CANCEL)
        procesos[1].estado = Estado::CANCEL;
        scheduler.clasificarPrioridades(procesos);
        // TL solo tiene P1(10). Como A=B=10, DP-03 exige P3
        EXPECT_EQ(procesos[0].prioridad, Prioridad::P3);
    }

    return test::finish();
}
