#include "concurrent/ConcurrentEngine.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

#include <chrono>
#include <thread>

namespace {

bool esperarRun(ConcurrentEngine& engine, int pid) {
    const auto limite =
        std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < limite) {
        if (engine.obtenerEstado().pidEjecutando == pid) {
            return true;
        }
        std::this_thread::yield();
    }
    return false;
}

}  // namespace

int main() {
    // Caso 1: la cuota se fija al despachar. Cambiarla durante una rafaga no
    // altera esa rafaga y se aplica desde el siguiente despacho (DP-04).
    {
        ConcurrentEngine engine(2048);
        engine.configurarQuantum(100);
        engine.configurarRetardoRafagaPruebasMs(40);

        Proceso p = crearProceso(1, "A", 100, Estado::READY, 5, 0);
        p.tiempoTotal = 2000;
        p.tiempoRestante = 2000;
        engine.agregarProceso(p);
        engine.pausar();

        std::thread paso([&engine]() { engine.siguientePaso(); });
        EXPECT_TRUE(esperarRun(engine, 1));
        engine.configurarQuantum(300);  // con la rafaga de 100 ms en curso
        paso.join();

        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.timeline.size(), 1U);
        EXPECT_EQ(s.timeline[0].fin - s.timeline[0].inicio, 100);
        EXPECT_EQ(s.quantumMs, 300);

        engine.siguientePaso();
        s = engine.obtenerEstado();
        EXPECT_EQ(s.timeline.size(), 2U);
        EXPECT_EQ(s.timeline[1].fin - s.timeline[1].inicio, 300);
    }

    // Caso 2: cada despacho toma la cuota vigente en ese momento.
    {
        ConcurrentEngine engine(2048);
        Proceso p = crearProceso(1, "A", 100, Estado::READY, 5, 0);
        p.tiempoTotal = 2000;
        p.tiempoRestante = 2000;
        engine.agregarProceso(p);
        engine.pausar();

        const std::int64_t cuotas[] = {50, 120, 10, 400};
        std::int64_t esperadoTotal = 0;
        for (const std::int64_t cuota : cuotas) {
            engine.configurarQuantum(cuota);
            engine.siguientePaso();
            esperadoTotal += cuota;
        }
        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.timeline.size(), 4U);
        for (std::size_t i = 0; i < s.timeline.size(); ++i) {
            EXPECT_EQ(s.timeline[i].fin - s.timeline[i].inicio, cuotas[i]);
        }
        EXPECT_EQ(s.tiempoSimulado, esperadoTotal);
    }

    return test::finish();
}
