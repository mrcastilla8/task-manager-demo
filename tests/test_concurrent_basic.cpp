#include "concurrent/ConcurrentEngine.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

int main() {
    // Caso 1: Creacion y configuracion inicial
    {
        ConcurrentEngine engine(2048);
        EXPECT_EQ(engine.memoriaTotal(), 2048);
        EXPECT_EQ(engine.memoriaUsada(), 0);
        EXPECT_EQ(engine.obtenerQuantum(), ConfiguracionPredeterminada::QUANTUM_MS);

        engine.configurarQuantum(300);
        EXPECT_EQ(engine.obtenerQuantum(), 300);

        EXPECT_THROW(engine.configurarQuantum(0), std::invalid_argument);
        EXPECT_THROW(engine.configurarQuantum(-100), std::invalid_argument);
    }

    // Caso 2: Validacion de admision y errores
    {
        ConcurrentEngine engine(1000);

        Proceso pInvalido; // pid -1, nombre vacio
        EXPECT_THROW(engine.agregarProceso(pInvalido), std::invalid_argument);

        Proceso p1 = crearProceso(1, "Proc1", 400);
        engine.agregarProceso(p1);
        EXPECT_EQ(engine.memoriaUsada(), 400);
        EXPECT_EQ(engine.cantidadWorkersActivos(), 1);

        // Duplicado lanza std::invalid_argument
        EXPECT_THROW(engine.agregarProceso(p1), std::invalid_argument);

        // Memoria insuficiente lanza std::runtime_error
        Proceso pExcede = crearProceso(2, "Excede", 700);
        EXPECT_THROW(engine.agregarProceso(pExcede), std::runtime_error);
    }

    // Caso 3: Paso a paso y ejecucion logica con worker real
    {
        ConcurrentEngine engine(2048);
        engine.configurarQuantum(500);

        Proceso p1 = crearProceso(1, "A", 200, Estado::READY, 8);
        p1.tiempoTotal = 1000;
        p1.tiempoRestante = 1000;

        Proceso p2 = crearProceso(2, "B", 200, Estado::READY, 5);
        p2.tiempoTotal = 500;
        p2.tiempoRestante = 500;

        engine.agregarProceso(p1);
        engine.agregarProceso(p2);

        // siguientePaso requiere estar en PAUSADO
        EXPECT_THROW(engine.siguientePaso(), std::logic_error);

        engine.pausar();
        EXPECT_EQ(engine.obtenerEstado().estadoEjecucion, EstadoEjecucion::PAUSADO);

        // Paso 1: P1 (prioridad 8) ejecuta 500 ms
        engine.siguientePaso();
        EstadoSistema s1 = engine.obtenerEstado();
        EXPECT_EQ(s1.tiempoSimulado, 500);
        EXPECT_EQ(s1.timeline.size(), 1U);
        EXPECT_EQ(s1.timeline[0].pid, 1);
        EXPECT_EQ(s1.timeline[0].inicio, 0);
        EXPECT_EQ(s1.timeline[0].fin, 500);

        // Paso 2: P1 agoto quantum, ahora P2 (prioridad 5) vs P1 (vuelve a prioridad 8 o aging)
        engine.siguientePaso();
        EstadoSistema s2 = engine.obtenerEstado();
        EXPECT_EQ(s2.tiempoSimulado, 1000);
        EXPECT_TRUE(s2.timeline.size() >= 2U);

        // Paso 3: Completar ejecucion
        engine.siguientePaso();
        EstadoSistema s3 = engine.obtenerEstado();
        EXPECT_TRUE(s3.tiempoSimulado >= 1500);
    }

    return test::finish();
}
