#include "core/v2/ListaProcesos.h"
#include "core/v2/Proceso.h"
#include "tests/TestSupport.h"

#include <utility>
#include <type_traits>
#include <cstdint>
#include <limits>

using namespace ConcurrentSimulator::v2;

namespace {
Proceso proceso(int pid, Estado estado = Estado::READY) {
    Proceso result = construirProcesoInicial(
        AltaProceso{pid, "p" + std::to_string(pid), pid * 10, 100, {}});
    result.estado = estado;
    return result;
}

void verificarProcesoSinCambios(const Proceso* actual,
                                const Proceso& esperado) {
    EXPECT_TRUE(actual != nullptr);
    if (actual == nullptr) {
        return;
    }
    EXPECT_EQ(actual->pid, esperado.pid);
    EXPECT_EQ(actual->estado, esperado.estado);
    EXPECT_EQ(actual->prioridad, esperado.prioridad);
    EXPECT_EQ(actual->tiempoCpuTotalMs, esperado.tiempoCpuTotalMs);
    EXPECT_EQ(actual->tiempoCpuConsumidoMs, esperado.tiempoCpuConsumidoMs);
    EXPECT_EQ(actual->tiempoCpuRestanteMs, esperado.tiempoCpuRestanteMs);
    EXPECT_EQ(actual->tiempoEsperaMs, esperado.tiempoEsperaMs);
    EXPECT_EQ(actual->ticketReady, esperado.ticketReady);
}
}  // namespace

int main() {
    ListaProcesos lista;
    static_assert(std::is_same<decltype(lista.buscar(1)), const Proceso*>::value,
                  "buscar solo debe exponer una vista const");
    EXPECT_TRUE(lista.vacia());
    EXPECT_EQ(lista.cantidad(), 0U);
    EXPECT_TRUE(lista.buscar(1) == nullptr);
    EXPECT_FALSE(lista.eliminar(1));

    EXPECT_TRUE(lista.agregar(proceso(1)));
    EXPECT_TRUE(lista.agregar(proceso(2, Estado::WAIT)));
    EXPECT_TRUE(lista.agregar(proceso(3)));
    EXPECT_FALSE(lista.agregar(proceso(2)));
    Proceso invalido = proceso(4);
    invalido.tiempoCpuRestanteMs = 99;
    EXPECT_FALSE(lista.agregar(invalido));
    invalido = proceso(4);
    invalido.pid = 0;
    EXPECT_FALSE(lista.agregar(invalido));
    const Proceso antesFinishInvalido = *lista.buscar(2);
    Proceso finishInvalido = proceso(5, Estado::FINISH);
    finishInvalido.tiempoCpuConsumidoMs = 50;
    finishInvalido.tiempoCpuRestanteMs = 50;
    EXPECT_FALSE(lista.agregar(finishInvalido));
    EXPECT_EQ(lista.cantidad(), 3U);
    EXPECT_TRUE(lista.buscar(5) == nullptr);
    verificarProcesoSinCambios(lista.buscar(2), antesFinishInvalido);
    EXPECT_EQ(lista.buscar(2)->estado, Estado::WAIT);
    EXPECT_EQ(lista.buscar(2)->tiempoCpuConsumidoMs, 0);
    EXPECT_EQ(lista.buscar(2)->tiempoCpuRestanteMs, 100);
    EXPECT_EQ(lista.buscar(2)->ticketReady, 0U);
    EXPECT_TRUE(lista.buscar(99) == nullptr);
    const auto todos = lista.obtenerTodos();
    EXPECT_EQ(todos.size(), 3U);
    EXPECT_EQ(todos[0].pid, 1);
    EXPECT_EQ(todos[1].pid, 2);
    EXPECT_EQ(todos[2].pid, 3);
    EXPECT_EQ(lista.obtenerPorEstado(Estado::READY).size(), 2U);

    // Transiciones operativas y terminalidad: los fallos conservan el estado.
    ListaProcesos terminales;
    EXPECT_TRUE(terminales.agregar(proceso(20)));
    EXPECT_TRUE(terminales.agregar(proceso(21, Estado::WAIT)));
    EXPECT_TRUE(terminales.agregar(proceso(22)));
    EXPECT_TRUE(terminales.agregar(proceso(23)));
    EXPECT_TRUE(terminales.agregar(proceso(24)));
    EXPECT_TRUE(terminales.actualizarEstado(20, Estado::RUN));
    EXPECT_TRUE(terminales.actualizarEstado(20, Estado::WAIT));
    EXPECT_TRUE(terminales.actualizarEstado(20, Estado::READY));
    EXPECT_TRUE(terminales.actualizarEstado(20, Estado::RUN));
    EXPECT_TRUE(terminales.actualizarCpu(20, 100, 0));
    EXPECT_TRUE(terminales.actualizarEstado(20, Estado::FINISH));
    EXPECT_TRUE(terminales.actualizarEstado(20, Estado::FINISH));
    for (Estado destino : {Estado::READY, Estado::RUN, Estado::WAIT,
                           Estado::CANCEL}) {
        EXPECT_FALSE(terminales.actualizarEstado(20, destino));
        EXPECT_EQ(terminales.buscar(20)->estado, Estado::FINISH);
    }
    EXPECT_TRUE(terminales.actualizarEstado(21, Estado::CANCEL));
    EXPECT_TRUE(terminales.actualizarEstado(21, Estado::CANCEL));
    for (Estado destino : {Estado::READY, Estado::RUN, Estado::WAIT,
                           Estado::FINISH}) {
        EXPECT_FALSE(terminales.actualizarEstado(21, destino));
        EXPECT_EQ(terminales.buscar(21)->estado, Estado::CANCEL);
    }
    EXPECT_FALSE(terminales.actualizarEstado(22, Estado::FINISH));
    EXPECT_EQ(terminales.buscar(22)->estado, Estado::READY);
    EXPECT_FALSE(terminales.actualizarEstado(22, Estado::WAIT));
    EXPECT_EQ(terminales.buscar(22)->estado, Estado::READY);
    EXPECT_TRUE(terminales.actualizarEstado(22, Estado::CANCEL));
    EXPECT_TRUE(terminales.actualizarEstado(24, Estado::RUN));
    EXPECT_TRUE(terminales.actualizarEstado(24, Estado::CANCEL));
    EXPECT_TRUE(terminales.actualizarTicketReady(23, 1));
    EXPECT_TRUE(terminales.actualizarTicketReady(
        23, (std::numeric_limits<std::uint64_t>::max)()));
    EXPECT_FALSE(terminales.actualizarTicketReady(23, 1));
    EXPECT_EQ(terminales.buscar(23)->ticketReady,
              (std::numeric_limits<std::uint64_t>::max)());

    EXPECT_TRUE(lista.actualizarEstado(2, Estado::READY));
    EXPECT_TRUE(lista.actualizarEstado(2, Estado::RUN));
    EXPECT_TRUE(lista.actualizarPrioridad(2, Prioridad::P1));
    EXPECT_TRUE(lista.actualizarCpu(2, 20, 80));
    EXPECT_TRUE(lista.actualizarTiempoEspera(2, 7));
    EXPECT_TRUE(lista.actualizarTicketReady(2, 42));
    EXPECT_TRUE(lista.actualizarCpu(2, 20, 80));  // idempotente
    EXPECT_FALSE(lista.actualizarCpu(2, 19, 81)); // CPU consumida retrocede
    EXPECT_FALSE(lista.actualizarCpu(2, 21, 80)); // restante sube sin conservar total
    EXPECT_FALSE(lista.actualizarCpu(2, -1, 101));
    EXPECT_FALSE(lista.actualizarCpu(2, 101, -1));
    EXPECT_FALSE(lista.actualizarCpu(2, 50, 40)); // no conserva el total
    EXPECT_FALSE(lista.actualizarCpu(2, 101, 0));
    EXPECT_FALSE(lista.actualizarTiempoEspera(2, -1));
    EXPECT_FALSE(lista.actualizarEstado(2, static_cast<Estado>(255)));
    EXPECT_FALSE(lista.actualizarPrioridad(2, static_cast<Prioridad>(0)));
    EXPECT_EQ(lista.buscar(2)->pid, 2);
    EXPECT_EQ(lista.buscar(2)->estado, Estado::RUN);
    EXPECT_EQ(lista.buscar(2)->prioridad, Prioridad::P1);
    EXPECT_EQ(lista.buscar(2)->tiempoCpuConsumidoMs, 20);
    EXPECT_EQ(lista.buscar(2)->tiempoCpuRestanteMs, 80);
    EXPECT_EQ(lista.buscar(2)->tiempoEsperaMs, 7);
    EXPECT_EQ(lista.buscar(2)->ticketReady, 42U);
    EXPECT_EQ(lista.buscar(2)->tiempoCpuConsumidoMs, 20);
    EXPECT_EQ(lista.buscar(2)->tiempoCpuRestanteMs, 80);
    EXPECT_FALSE(lista.actualizarTicketReady(2, 41));
    EXPECT_EQ(lista.buscar(2)->ticketReady, 42U);
    EXPECT_TRUE(lista.actualizarTicketReady(2, 42)); // misma asignación
    EXPECT_TRUE(lista.actualizarTicketReady(2, 43));
    EXPECT_EQ(lista.buscar(2)->ticketReady, 43U);

    // H-09: solo RUN admite progreso CPU; una repetición exacta es inocua.
    ListaProcesos cpuPorEstado;
    Proceso readyCpu = proceso(30);
    Proceso waitCpu = proceso(31, Estado::WAIT);
    Proceso cancelCpu = proceso(32);
    Proceso finishCpu = proceso(33, Estado::FINISH);
    finishCpu.tiempoCpuConsumidoMs = 100;
    finishCpu.tiempoCpuRestanteMs = 0;
    Proceso runCpu = proceso(34);
    EXPECT_TRUE(cpuPorEstado.agregar(readyCpu));
    EXPECT_TRUE(cpuPorEstado.agregar(waitCpu));
    EXPECT_TRUE(cpuPorEstado.agregar(cancelCpu));
    EXPECT_TRUE(cpuPorEstado.agregar(finishCpu));
    EXPECT_TRUE(cpuPorEstado.agregar(runCpu));
    EXPECT_TRUE(cpuPorEstado.actualizarEstado(32, Estado::RUN));
    EXPECT_TRUE(cpuPorEstado.actualizarCpu(32, 10, 90));
    EXPECT_TRUE(cpuPorEstado.actualizarEstado(32, Estado::CANCEL));
    EXPECT_TRUE(cpuPorEstado.actualizarEstado(34, Estado::RUN));

    auto rechazarSinMutar = [&](int pid, std::int64_t consumido,
                                std::int64_t restante) {
        const Proceso antes = *cpuPorEstado.buscar(pid);
        EXPECT_FALSE(cpuPorEstado.actualizarCpu(pid, consumido, restante));
        verificarProcesoSinCambios(cpuPorEstado.buscar(pid), antes);
    };
    auto aceptarIdempotente = [&](int pid) {
        const Proceso antes = *cpuPorEstado.buscar(pid);
        EXPECT_TRUE(cpuPorEstado.actualizarCpu(
            pid, antes.tiempoCpuConsumidoMs, antes.tiempoCpuRestanteMs));
        verificarProcesoSinCambios(cpuPorEstado.buscar(pid), antes);
    };
    aceptarIdempotente(30);  // READY
    aceptarIdempotente(31);  // WAIT
    aceptarIdempotente(32);  // CANCEL
    aceptarIdempotente(33);  // FINISH
    rechazarSinMutar(30, 10, 90);  // READY
    rechazarSinMutar(31, 10, 90);  // WAIT
    rechazarSinMutar(32, 20, 80);  // CANCEL
    rechazarSinMutar(33, 60, 40);  // FINISH

    Proceso antesRun = *cpuPorEstado.buscar(34);
    EXPECT_TRUE(cpuPorEstado.actualizarCpu(34, 0, 100));  // idempotente en RUN
    verificarProcesoSinCambios(cpuPorEstado.buscar(34), antesRun);
    EXPECT_TRUE(cpuPorEstado.actualizarCpu(34, 10, 90));
    EXPECT_TRUE(cpuPorEstado.actualizarCpu(34, 10, 90));  // idempotente
    const Proceso runAvanzado = *cpuPorEstado.buscar(34);
    EXPECT_FALSE(cpuPorEstado.actualizarCpu(34, 9, 91));  // retroceso
    verificarProcesoSinCambios(cpuPorEstado.buscar(34), runAvanzado);
    EXPECT_TRUE(cpuPorEstado.actualizarCpu(34, 100, 0));  // agotamiento exacto
    EXPECT_TRUE(cpuPorEstado.actualizarEstado(34, Estado::FINISH));
    const Proceso finishAgotado = *cpuPorEstado.buscar(34);
    EXPECT_TRUE(cpuPorEstado.actualizarCpu(34, 100, 0));  // idempotente terminal
    verificarProcesoSinCambios(cpuPorEstado.buscar(34), finishAgotado);

    EXPECT_TRUE(lista.eliminar(2));  // medio
    EXPECT_TRUE(lista.eliminar(1));  // primero
    EXPECT_EQ(lista.buscar(3)->pid, 3);
    EXPECT_TRUE(lista.agregar(proceso(4)));
    EXPECT_TRUE(lista.eliminar(4));  // último
    EXPECT_TRUE(lista.eliminar(3));  // único restante
    EXPECT_TRUE(lista.vacia());
    EXPECT_FALSE(lista.eliminar(3));

    EXPECT_TRUE(lista.agregar(proceso(10)));
    EXPECT_TRUE(lista.agregar(proceso(11)));
    ListaProcesos copia(lista);
    EXPECT_EQ(copia.cantidad(), 2U);
    EXPECT_TRUE(copia.actualizarEstado(10, Estado::RUN));
    EXPECT_TRUE(copia.actualizarEstado(10, Estado::WAIT));
    EXPECT_EQ(lista.buscar(10)->nombre, "p10");
    EXPECT_EQ(lista.buscar(10)->estado, Estado::READY);
    EXPECT_EQ(copia.buscar(10)->estado, Estado::WAIT);
    ListaProcesos asignada;
    asignada.agregar(proceso(99));
    asignada = lista;
    asignada.eliminar(11);
    EXPECT_EQ(lista.cantidad(), 2U);
    EXPECT_EQ(asignada.cantidad(), 1U);
    asignada = asignada;
    EXPECT_EQ(asignada.cantidad(), 1U);

    ListaProcesos movida(std::move(copia));
    EXPECT_EQ(copia.cantidad(), 0U);
    EXPECT_TRUE(copia.vacia());
    EXPECT_EQ(movida.cantidad(), 2U);
    ListaProcesos destino;
    destino.agregar(proceso(50));
    destino = std::move(movida);
    EXPECT_EQ(movida.cantidad(), 0U);
    EXPECT_EQ(destino.cantidad(), 2U);
    EXPECT_TRUE(destino.eliminar(11));
    EXPECT_TRUE(destino.agregar(proceso(12)));
    EXPECT_EQ(destino.buscar(12)->pid, 12);

    ListaProcesos aplicarLote;
    Proceso procesoActivo = proceso(101);
    Proceso procesoWait = proceso(102, Estado::WAIT);
    Proceso procesoTerminal = proceso(103, Estado::CANCEL);
    procesoActivo.tamanioBytes = 12;
    procesoWait.tamanioBytes = 96;
    procesoTerminal.prioridad = Prioridad::P2;
    EXPECT_TRUE(aplicarLote.agregar(procesoActivo));
    EXPECT_TRUE(aplicarLote.agregar(procesoWait));
    EXPECT_TRUE(aplicarLote.agregar(procesoTerminal));

    auto vistaCalculada = aplicarLote.obtenerTodos();
    vistaCalculada[0].prioridad = Prioridad::P1;
    vistaCalculada[1].prioridad = Prioridad::P3;
    EXPECT_TRUE(aplicarLote.aplicarPrioridades(vistaCalculada));
    EXPECT_EQ(aplicarLote.buscar(101)->prioridad, Prioridad::P1);
    EXPECT_EQ(aplicarLote.buscar(102)->prioridad, Prioridad::P3);
    EXPECT_EQ(aplicarLote.buscar(103)->prioridad, Prioridad::P2);

    auto loteInvalido = aplicarLote.obtenerTodos();
    loteInvalido[0].prioridad = Prioridad::P3;
    loteInvalido[1].pid = 999;
    EXPECT_FALSE(aplicarLote.aplicarPrioridades(loteInvalido));
    EXPECT_EQ(aplicarLote.buscar(101)->prioridad, Prioridad::P1);
    EXPECT_EQ(aplicarLote.buscar(102)->prioridad, Prioridad::P3);

    auto vistaObsoleta = aplicarLote.obtenerTodos();
    EXPECT_TRUE(aplicarLote.actualizarTicketReady(101, 1));
    vistaObsoleta[0].prioridad = Prioridad::P2;
    EXPECT_FALSE(aplicarLote.aplicarPrioridades(vistaObsoleta));
    EXPECT_EQ(aplicarLote.buscar(101)->prioridad, Prioridad::P1);

    ListaProcesos repetida;
    for (int pid = 1; pid <= 500; ++pid) {
        EXPECT_TRUE(repetida.agregar(proceso(pid + 20000)));
    }
    for (int pid = 1; pid <= 500; ++pid) {
        EXPECT_TRUE(repetida.eliminar(pid + 20000));
        EXPECT_FALSE(repetida.eliminar(pid + 20000));
    }
    EXPECT_TRUE(repetida.vacia());

    ListaProcesos grande;
    constexpr int cantidadGrande = 5000;
    for (int pid = 1; pid <= cantidadGrande; ++pid) {
        EXPECT_TRUE(grande.agregar(proceso(pid + 1000)));
    }
    EXPECT_EQ(grande.cantidad(), static_cast<std::size_t>(cantidadGrande));
    // La destrucción sale de scope y desengancha nodos iterativamente.

    return test::finish();
}
