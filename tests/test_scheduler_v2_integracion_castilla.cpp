#include "core/ColaProcesosV2.h"
#include "scheduler/SchedulerV2.h"
#include "tests/FixturesV2.h"
#include "tests/TestSupport.h"

#include <climits>
#include <stdexcept>
#include <vector>

using namespace ConcurrentSimulator::v2;

int main() {
    Scheduler scheduler;
    AsignadorTickets asignador;

    // =========================================================================
    // Escenario 1: Recalculacion y persistencia de prioridades en ColaProcesos
    // sin alterar estados, CPU, tickets ni otros campos de los procesos.
    // =========================================================================
    {
        ColaProcesos cola;

        Proceso p1 = crearProcesoV2(1, "ProcP1", 12, Prioridad::P3, asignador.asignar(), Estado::READY);
        p1.tiempoCpuRestanteMs = 1500;
        p1.tiempoCpuConsumidoMs = 500;
        p1.tiempoEsperaMs = 120;

        Proceso p2 = crearProcesoV2(2, "ProcP2", 40, Prioridad::P3, asignador.asignar(), Estado::RUN);
        p2.tiempoCpuRestanteMs = 1800;
        p2.tiempoCpuConsumidoMs = 200;
        p2.tiempoEsperaMs = 0;

        Proceso p3 = crearProcesoV2(3, "ProcP3", 96, Prioridad::P1, asignador.asignar(), Estado::WAIT);
        p3.tiempoCpuRestanteMs = 2000;
        p3.tiempoCpuConsumidoMs = 0;
        p3.tiempoEsperaMs = 450;

        cola.agregar(p1);
        cola.agregar(p2);
        cola.agregar(p3);

        EXPECT_EQ(cola.cantidad(), 3U);

        // La cola delega el calculo de prioridades exclusivamente al Scheduler
        cola.recalcularPrioridades(scheduler);

        const Proceso* proc1 = cola.buscar(1);
        const Proceso* proc2 = cola.buscar(2);
        const Proceso* proc3 = cola.buscar(3);

        EXPECT_TRUE(proc1 != nullptr);
        EXPECT_TRUE(proc2 != nullptr);
        EXPECT_TRUE(proc3 != nullptr);

        // Verificacion de prioridades calculadas por el Scheduler
        // Poblacion TL = {12, 40, 96} -> minA=12, maxB=96, D=84
        // 12 -> P1, 40 -> P2, 96 -> P3
        EXPECT_EQ(proc1->prioridad, Prioridad::P1);
        EXPECT_EQ(proc2->prioridad, Prioridad::P2);
        EXPECT_EQ(proc3->prioridad, Prioridad::P3);

        // Verificacion estricta: ningun otro dato fue alterado
        EXPECT_EQ(proc1->estado, Estado::READY);
        EXPECT_EQ(proc1->tiempoCpuRestanteMs, 1500);
        EXPECT_EQ(proc1->tiempoCpuConsumidoMs, 500);
        EXPECT_EQ(proc1->tiempoEsperaMs, 120);
        EXPECT_EQ(proc1->ticketReady, 1U);

        EXPECT_EQ(proc2->estado, Estado::RUN);
        EXPECT_EQ(proc2->tiempoCpuRestanteMs, 1800);
        EXPECT_EQ(proc2->tiempoCpuConsumidoMs, 200);
        EXPECT_EQ(proc2->tiempoEsperaMs, 0);
        EXPECT_EQ(proc2->ticketReady, 2U);

        EXPECT_EQ(proc3->estado, Estado::WAIT);
        EXPECT_EQ(proc3->tiempoCpuRestanteMs, 2000);
        EXPECT_EQ(proc3->tiempoCpuConsumidoMs, 0);
        EXPECT_EQ(proc3->tiempoEsperaMs, 450);
        EXPECT_EQ(proc3->ticketReady, 3U);
    }

    // =========================================================================
    // Escenario 2: Alta rechazada (validacion y memoria) no altera TL ni prioridades (DP-02c)
    // =========================================================================
    {
        ColaProcesos cola;
        cola.agregar(crearProcesoV2(10, "BaseA", 20, Prioridad::P3, asignador.asignar(), Estado::READY));
        cola.agregar(crearProcesoV2(20, "BaseB", 60, Prioridad::P3, asignador.asignar(), Estado::READY));

        cola.recalcularPrioridades(scheduler);
        EXPECT_EQ(cola.buscar(10)->prioridad, Prioridad::P1);
        EXPECT_EQ(cola.buscar(20)->prioridad, Prioridad::P3);

        // Rechazo por tamano no positivo (<= 0)
        EXPECT_THROW(cola.agregar(crearProcesoV2(30, "InvalidoTam0", 0)), std::invalid_argument);
        EXPECT_THROW(cola.agregar(crearProcesoV2(31, "InvalidoTamNeg", -50)), std::invalid_argument);

        // Rechazo por PID repetido o no positivo
        EXPECT_THROW(cola.agregar(crearProcesoV2(10, "PIDDuplicado", 50)), std::invalid_argument);
        EXPECT_THROW(cola.agregar(crearProcesoV2(0, "PIDZero", 50)), std::invalid_argument);

        // Verificar que la cola y las prioridades de TL no sufrieron ninguna mutacion
        EXPECT_EQ(cola.cantidad(), 2U);
        EXPECT_EQ(cola.buscar(10)->prioridad, Prioridad::P1);
        EXPECT_EQ(cola.buscar(20)->prioridad, Prioridad::P3);
    }

    // =========================================================================
    // Escenario 3: Finalizacion (FINISH) de proceso y recalculacion de TL (DP-02a, DP-02b)
    // =========================================================================
    {
        ColaProcesos cola;
        cola.agregar(crearProcesoV2(1, "P1", 10, Prioridad::P1, asignador.asignar(), Estado::READY));
        cola.agregar(crearProcesoV2(2, "P2", 20, Prioridad::P1, asignador.asignar(), Estado::READY));
        cola.agregar(crearProcesoV2(3, "P3", 100, Prioridad::P1, asignador.asignar(), Estado::READY));

        cola.recalcularPrioridades(scheduler);
        // Con TL={10, 20, 100}: D=90 -> 10 es P1, 20 es P1 (3*10 < 90), 100 es P3
        EXPECT_EQ(cola.buscar(1)->prioridad, Prioridad::P1);
        EXPECT_EQ(cola.buscar(2)->prioridad, Prioridad::P1);
        EXPECT_EQ(cola.buscar(3)->prioridad, Prioridad::P3);

        // P3 termina ráfagas y transiciona a FINISH
        cola.buscar(3)->estado = Estado::FINISH;

        // Disparador DP-02b: recalcular tras FINISH
        cola.recalcularPrioridades(scheduler);

        // P3 ya no forma parte de TL (DP-02a) y conserva su prioridad previa
        EXPECT_EQ(cola.buscar(3)->prioridad, Prioridad::P3);

        // TL activa es {10, 20}: D = 10 -> 10 es P1, 20 es P3 (maximo)
        EXPECT_EQ(cola.buscar(1)->prioridad, Prioridad::P1);
        EXPECT_EQ(cola.buscar(2)->prioridad, Prioridad::P3);
    }

    // =========================================================================
    // Escenario 4: Cancelacion (CANCEL) de proceso y recalculacion de TL (DP-02a, DP-02b)
    // =========================================================================
    {
        ColaProcesos cola;
        cola.agregar(crearProcesoV2(1, "P1", 10, Prioridad::P1, asignador.asignar(), Estado::READY));
        cola.agregar(crearProcesoV2(2, "P2", 20, Prioridad::P3, asignador.asignar(), Estado::READY));

        cola.recalcularPrioridades(scheduler);
        EXPECT_EQ(cola.buscar(1)->prioridad, Prioridad::P1);
        EXPECT_EQ(cola.buscar(2)->prioridad, Prioridad::P3);

        // Cancelar P2
        cola.buscar(2)->estado = Estado::CANCEL;

        // Disparador DP-02b: recalcular tras CANCEL
        cola.recalcularPrioridades(scheduler);

        // P2 conserva su prioridad
        EXPECT_EQ(cola.buscar(2)->prioridad, Prioridad::P3);

        // TL solo tiene P1 (A=B=10). Segun regla DP-03, A=B retorna P3!
        EXPECT_EQ(cola.buscar(1)->prioridad, Prioridad::P3);
    }

    // =========================================================================
    // Escenario 5: Renovacion de tickets READY, alternancia FIFO y descarte
    //              de procesos con ticket sin asignar (DP-05)
    // =========================================================================
    {
        ColaProcesos cola;
        AsignadorTickets asignadorTurnos(1);

        // 1. Proceso READY con ticket sin asignar (ticketReady == 0) no debe ser despachado
        Proceso sinTicket = crearProcesoV2(99, "SinTicket", 50, Prioridad::P3,
                                           Scheduler::TICKET_SIN_ASIGNAR, Estado::READY);
        cola.agregar(sinTicket);

        EXPECT_EQ(scheduler.seleccionarSiguiente(cola.procesos()), -1);
        EXPECT_TRUE(scheduler.ordenarColaPrioridad(cola.procesos()).empty());

        // 2. Asignar ticket valido al admitir
        asignadorTurnos.renovarTicket(*cola.buscar(99));
        EXPECT_EQ(cola.buscar(99)->ticketReady, 1U);
        EXPECT_EQ(scheduler.seleccionarSiguiente(cola.procesos()), 99);

        // 3. Dos procesos de igual prioridad (P2): Round Robin por ticket ascendente
        ColaProcesos colaRR;
        Proceso pa = crearProcesoV2(1, "ProcA", 50, Prioridad::P2, asignadorTurnos.asignar(), Estado::READY); // ticket 2
        Proceso pb = crearProcesoV2(2, "ProcB", 50, Prioridad::P2, asignadorTurnos.asignar(), Estado::READY); // ticket 3
        colaRR.agregar(pa);
        colaRR.agregar(pb);

        // Primer despacho: selecciona ProcA (ticket 2 < ticket 3)
        EXPECT_EQ(scheduler.seleccionarSiguiente(colaRR.procesos()), 1);

        // ProcA pasa a RUN y agota su cuota de 20 ms -> vuelve a READY con nuevo ticket
        colaRR.buscar(1)->estado = Estado::READY;
        asignadorTurnos.renovarTicket(*colaRR.buscar(1)); // ticket 4

        // Segundo despacho: ahora ProcB tiene menor ticket (ticket 3 < ticket 4) -> selecciona ProcB
        EXPECT_EQ(scheduler.seleccionarSiguiente(colaRR.procesos()), 2);

        // ProcB pasa a RUN, se bloquea en I/O (WAIT) y luego despierta a READY -> ticket nuevo
        colaRR.buscar(2)->estado = Estado::WAIT;
        // Mientras B esta en WAIT, solo A esta READY
        EXPECT_EQ(scheduler.seleccionarSiguiente(colaRR.procesos()), 1);

        // B completa I/O y vuelve a READY
        colaRR.buscar(2)->estado = Estado::READY;
        asignadorTurnos.renovarTicket(*colaRR.buscar(2)); // ticket 5

        // Ahora ProcA tiene ticket 4 y ProcB tiene ticket 5 -> selecciona ProcA
        EXPECT_EQ(scheduler.seleccionarSiguiente(colaRR.procesos()), 1);

        // 4. Verificacion de control de desbordamiento (overflow de tickets uint64_t)
        AsignadorTickets asignadorLimite(UINT64_MAX - 1);
        EXPECT_EQ(asignadorLimite.asignar(), UINT64_MAX - 1);
        EXPECT_THROW(asignadorLimite.asignar(), std::overflow_error);
    }

    return test::finish();
}
