#include "concurrent/ConcurrentEngine.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

int main() {
    // Caso 1: Destruccion de engine con workers activos
    {
        ConcurrentEngine engine(2048);
        for (int i = 1; i <= 5; ++i) {
            engine.agregarProceso(crearProceso(i, "Proc" + std::to_string(i), 50));
        }
        EXPECT_EQ(engine.cantidadWorkersActivos(), 5);
        // Al salir del bloque, el destructor debe liberar y joinear todos los threads sin deadlock ni fugas
    }

    // Caso 2: Reiniciar detiene workers y reinicia estado
    {
        ConcurrentEngine engine(1024);
        engine.configurarQuantum(250);

        Proceso p1 = crearProceso(1, "A", 200, Estado::READY, 5);
        p1.tiempoTotal = 1000;
        p1.tiempoRestante = 1000;

        engine.agregarProceso(p1);
        EXPECT_EQ(engine.cantidadWorkersActivos(), 1);

        engine.pausar();
        engine.siguientePaso();
        EXPECT_EQ(engine.obtenerEstado().tiempoSimulado, 250);

        // Reinicio completo
        engine.reiniciar();
        EXPECT_EQ(engine.cantidadWorkersActivos(), 0);

        EstadoSistema reiniciado = engine.obtenerEstado();
        EXPECT_EQ(reiniciado.tiempoSimulado, 0);
        EXPECT_EQ(reiniciado.procesos.size(), 0U);
        EXPECT_EQ(reiniciado.timeline.size(), 0U);
        EXPECT_EQ(reiniciado.eventos.size(), 0U);
        EXPECT_EQ(reiniciado.memoriaUsada, 0);
        EXPECT_EQ(reiniciado.memoriaTotal, 1024); // Capacidad conservada
        EXPECT_EQ(reiniciado.quantumMs, 250);     // Quantum conservado
        EXPECT_EQ(reiniciado.estadoEjecucion, EstadoEjecucion::DETENIDO);

        // Se pueden agregar nuevos procesos luego de reiniciar
        Proceso p2 = crearProceso(10, "Nuevo", 150);
        engine.agregarProceso(p2);
        EXPECT_EQ(engine.cantidadWorkersActivos(), 1);
        EXPECT_EQ(engine.memoriaUsada(), 150);
    }

    return test::finish();
}
