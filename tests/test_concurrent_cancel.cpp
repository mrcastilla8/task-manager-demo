#include "concurrent/ConcurrentEngine.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

#include <algorithm>

int main() {
    ConcurrentEngine engine(2048);

    Proceso p1 = crearProceso(1, "A", 300, Estado::READY, 5);
    p1.tiempoTotal = 1000;
    p1.tiempoRestante = 1000;

    Proceso p2 = crearProceso(2, "B", 400, Estado::READY, 7);
    p2.tiempoTotal = 1000;
    p2.tiempoRestante = 1000;

    engine.agregarProceso(p1);
    engine.agregarProceso(p2);
    EXPECT_EQ(engine.memoriaUsada(), 700);

    // Caso 1: Cancelar un proceso que no existe (idempotente)
    engine.cancelarProceso(999);
    EXPECT_EQ(engine.memoriaUsada(), 700);

    // Caso 2: Cancelar proceso en READY
    engine.cancelarProceso(1);
    EXPECT_EQ(engine.memoriaUsada(), 400); // se libera memoria de p1

    EstadoSistema s1 = engine.obtenerEstado();
    const auto itP1 = std::find_if(s1.procesos.begin(), s1.procesos.end(),
                                   [](const Proceso& p) { return p.pid == 1; });
    EXPECT_TRUE(itP1 != s1.procesos.end());
    EXPECT_EQ(itP1->estado, Estado::CANCEL);

    // Caso 3: Cancelar nuevamente el mismo proceso ya cancelado (idempotente)
    engine.cancelarProceso(1);
    EXPECT_EQ(engine.memoriaUsada(), 400);

    // Caso 4: Siguiente paso solo ejecuta P2
    engine.pausar();
    engine.siguientePaso();
    EstadoSistema s2 = engine.obtenerEstado();
    EXPECT_EQ(s2.timeline.size(), 1U);
    EXPECT_EQ(s2.timeline[0].pid, 2);

    // Caso 5: Cancelar P2
    engine.cancelarProceso(2);
    EXPECT_EQ(engine.memoriaUsada(), 0);
    EXPECT_EQ(engine.obtenerEstado().estadoEjecucion, EstadoEjecucion::FINALIZADO);

    // Caso 6: cancelar un proceso en WAIT libera memoria y detiene su worker.
    {
        ConcurrentEngine motor(2048);
        Proceso conIO = crearProceso(1, "ConIO", 300, Estado::READY, 5, 1);
        conIO.tiempoTotal = 1000;
        conIO.tiempoRestante = 1000;
        conIO.operacionIO = OperacionIO{true, 200, 400};
        Proceso otro = crearProceso(2, "Otro", 100, Estado::READY, 3, 2);
        otro.tiempoTotal = 100;
        otro.tiempoRestante = 100;
        motor.agregarProceso(conIO);
        motor.agregarProceso(otro);
        motor.pausar();

        motor.siguientePaso();  // P1 ejecuta 200 ms y pasa a WAIT
        const EstadoSistema esperando = motor.obtenerEstado();
        const auto itWait = std::find_if(
            esperando.procesos.begin(), esperando.procesos.end(),
            [](const Proceso& p) { return p.pid == 1; });
        EXPECT_TRUE(itWait != esperando.procesos.end());
        if (itWait != esperando.procesos.end()) {
            EXPECT_EQ(itWait->estado, Estado::WAIT);
        }
        EXPECT_EQ(motor.memoriaUsada(), 400);
        EXPECT_EQ(motor.cantidadWorkersActivos(), 2);

        motor.cancelarProceso(1);
        EXPECT_EQ(motor.memoriaUsada(), 100);
        EXPECT_EQ(motor.cantidadWorkersActivos(), 1);

        // Cancelar de nuevo no repite el evento.
        motor.cancelarProceso(1);
        const EstadoSistema trasCancelar = motor.obtenerEstado();
        const auto cancelaciones = std::count_if(
            trasCancelar.eventos.begin(), trasCancelar.eventos.end(),
            [](const Evento& e) {
                return e.pid == 1 && e.tipo == TipoEvento::PROCESS_CANCELLED;
            });
        EXPECT_EQ(cancelaciones, 1);

        // El proceso restante sigue ejecutandose y el motor finaliza.
        motor.siguientePaso();
        EXPECT_EQ(motor.obtenerEstado().estadoEjecucion,
                  EstadoEjecucion::FINALIZADO);
    }

    // Caso 7: cancelar un proceso FINISH se ignora y no cambia su estado.
    {
        ConcurrentEngine motor(2048);
        Proceso unico = crearProceso(1, "Corto", 100, Estado::READY, 5, 1);
        unico.tiempoTotal = 100;
        unico.tiempoRestante = 100;
        motor.agregarProceso(unico);
        motor.pausar();
        motor.siguientePaso();

        const EstadoSistema fin = motor.obtenerEstado();
        EXPECT_EQ(fin.procesos[0].estado, Estado::FINISH);

        motor.cancelarProceso(1);
        const EstadoSistema despues = motor.obtenerEstado();
        EXPECT_EQ(despues.procesos[0].estado, Estado::FINISH);
        EXPECT_EQ(despues.eventos.size(), fin.eventos.size());
    }

    return test::finish();
}
