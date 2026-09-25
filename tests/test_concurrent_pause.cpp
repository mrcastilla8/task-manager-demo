#include "concurrent/ConcurrentEngine.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

#include <chrono>
#include <thread>

int main() {
    ConcurrentEngine engine(2048);
    engine.configurarIntervaloCoordinadorMs(1);
    engine.configurarQuantum(200);

    Proceso p1 = crearProceso(1, "A", 100, Estado::READY, 5);
    p1.tiempoTotal = 2000;
    p1.tiempoRestante = 2000;

    engine.agregarProceso(p1);

    // Caso 1: Idempotencia de pausar()
    engine.pausar();
    EXPECT_EQ(engine.obtenerEstado().estadoEjecucion, EstadoEjecucion::PAUSADO);
    engine.pausar();
    EXPECT_EQ(engine.obtenerEstado().estadoEjecucion, EstadoEjecucion::PAUSADO);

    // Caso 2: siguientePaso funciona en PAUSADO
    engine.siguientePaso();
    EXPECT_EQ(engine.obtenerEstado().tiempoSimulado, 200);

    // Caso 3: Idempotencia de iniciar()
    engine.iniciar();
    EXPECT_EQ(engine.obtenerEstado().estadoEjecucion, EstadoEjecucion::EJECUTANDO);
    engine.iniciar();
    EXPECT_EQ(engine.obtenerEstado().estadoEjecucion, EstadoEjecucion::EJECUTANDO);

    // Mientras ejecuta, siguientePaso debe lanzar logic_error
    EXPECT_THROW(engine.siguientePaso(), std::logic_error);

    // Permitir avance continuo del hilo coordinador
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Pausar y verificar que el tiempo simulado avanzo
    engine.pausar();
    EXPECT_EQ(engine.obtenerEstado().estadoEjecucion, EstadoEjecucion::PAUSADO);
    const std::int64_t tiempoTrasPausa = engine.obtenerEstado().tiempoSimulado;
    EXPECT_TRUE(tiempoTrasPausa > 200);

    // En pausa, los workers quedan dormidos en condition_variable sin avanzar tiempo
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_EQ(engine.obtenerEstado().tiempoSimulado, tiempoTrasPausa);

    return test::finish();
}
