#include "core/v2/AdmisorProcesos.h"
#include "core/v2/ListaProcesos.h"
#include "core/v2/Memoria.h"
#include "scheduler/SchedulerV2.h"
#include "tests/TestSupport.h"

#include <vector>

using namespace ConcurrentSimulator::v2;

namespace {

bool clasificarYAplicar(ListaProcesos& procesos, const Scheduler& scheduler) {
    auto vista = procesos.obtenerTodos();
    scheduler.clasificarPrioridades(vista);
    return procesos.aplicarPrioridades(vista);
}

bool asignarTicket(ListaProcesos& procesos, AsignadorTickets& asignador,
                   int pid) {
    const Proceso* encontrado = procesos.buscar(pid);
    if (encontrado == nullptr) {
        return false;
    }
    Proceso copia = *encontrado;
    asignador.renovarTicket(copia);
    return procesos.actualizarTicketReady(pid, copia.ticketReady);
}

}  // namespace

int main() {
    ListaProcesos procesos;
    Memoria memoria(500);
    Scheduler scheduler;
    AsignadorTickets tickets;

    EXPECT_EQ(admitirProceso({1, "pequeno", 12, 50, {}}, procesos, memoria),
              ResultadoAdmision::ACEPTADO);
    EXPECT_EQ(admitirProceso({2, "medio-a", 40, 50, {}}, procesos, memoria),
              ResultadoAdmision::ACEPTADO);
    EXPECT_EQ(admitirProceso({3, "medio-b", 40, 50, {}}, procesos, memoria),
              ResultadoAdmision::ACEPTADO);
    EXPECT_EQ(admitirProceso({4, "grande", 96, 50, {}}, procesos, memoria),
              ResultadoAdmision::ACEPTADO);
    EXPECT_EQ(memoria.utilizadaBytes(), 188);

    EXPECT_TRUE(asignarTicket(procesos, tickets, 1));
    EXPECT_TRUE(asignarTicket(procesos, tickets, 2));
    EXPECT_TRUE(asignarTicket(procesos, tickets, 3));
    EXPECT_TRUE(asignarTicket(procesos, tickets, 4));
    EXPECT_TRUE(clasificarYAplicar(procesos, scheduler));

    EXPECT_EQ(procesos.buscar(1)->prioridad, Prioridad::P1);
    EXPECT_EQ(procesos.buscar(2)->prioridad, Prioridad::P2);
    EXPECT_EQ(procesos.buscar(3)->prioridad, Prioridad::P2);
    EXPECT_EQ(procesos.buscar(4)->prioridad, Prioridad::P3);
    EXPECT_EQ(scheduler.seleccionarSiguiente(procesos.obtenerTodos()), 4);

    EXPECT_TRUE(procesos.actualizarEstado(4, Estado::CANCEL));
    EXPECT_TRUE(memoria.liberar(4));
    EXPECT_TRUE(clasificarYAplicar(procesos, scheduler));

    EXPECT_EQ(procesos.buscar(4)->estado, Estado::CANCEL);
    EXPECT_EQ(memoria.utilizadaBytes(), 92);
    EXPECT_EQ(procesos.buscar(1)->prioridad, Prioridad::P1);
    EXPECT_EQ(procesos.buscar(2)->prioridad, Prioridad::P3);
    EXPECT_EQ(procesos.buscar(3)->prioridad, Prioridad::P3);
    EXPECT_EQ(scheduler.seleccionarSiguiente(procesos.obtenerTodos()), 2);
    EXPECT_EQ(scheduler.ordenarColaPrioridad(procesos.obtenerTodos()),
              (std::vector<int>{2, 3, 1}));

    return test::finish();
}
