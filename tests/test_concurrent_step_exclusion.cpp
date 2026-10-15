#include "concurrent/ConcurrentEngine.h"
#include "tests/Fixtures.h"
#include "tests/SnapshotInvariants.h"
#include "tests/TestSupport.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <vector>

namespace {

bool esperarHasta(const std::function<bool()>& condicion, int limiteMs = 5000) {
    const auto limite = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(limiteMs);
    while (std::chrono::steady_clock::now() < limite) {
        if (condicion()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return condicion();
}

void cargarProcesos(ConcurrentEngine& engine, int cantidad) {
    for (int i = 1; i <= cantidad; ++i) {
        Proceso p = crearProceso(i, "P" + std::to_string(i), 100, Estado::READY,
                                 5, i);
        p.tiempoTotal = 600;
        p.tiempoRestante = 600;
        engine.agregarProceso(p);
    }
}

}  // namespace

int main() {
    ConcurrentEngine engine(10000);
    engine.configurarIntervaloCoordinadorMs(0);
    engine.configurarQuantum(100);
    // Ensancha la ventana de la rafaga para que las carreras sean reproducibles.
    engine.configurarRetardoRafagaPruebasMs(15);

    // Monitor: valida invariantes del snapshot durante toda la prueba.
    std::atomic<bool> monitorActivo{true};
    std::atomic<int> violaciones{0};
    std::thread monitor([&]() {
        while (monitorActivo.load()) {
            const int malas = invariantesVioladas(engine.obtenerEstado());
            if (malas > 0) {
                violaciones.fetch_add(malas);
            }
            std::this_thread::yield();
        }
    });

    // Caso 1: dos hilos ejecutan siguientePaso() a la vez; las unidades se
    // serializan (nunca dos procesos en RUN) y no se pierde ninguna.
    {
        cargarProcesos(engine, 4);
        engine.pausar();

        auto pasos = [&engine]() {
            for (int i = 0; i < 3; ++i) {
                engine.siguientePaso();
            }
        };
        std::thread t1(pasos);
        std::thread t2(pasos);
        t1.join();
        t2.join();

        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.timeline.size(), 6U);
        EXPECT_EQ(s.tiempoSimulado, 600);
        EXPECT_EQ(s.pidEjecutando, -1);
    }

    // Caso 2: iniciar() mientras otro hilo ejecuta un siguientePaso() no abre
    // una segunda unidad; el coordinador espera a que cierre la primera.
    {
        engine.reiniciar();
        cargarProcesos(engine, 4);
        engine.pausar();

        std::thread manual([&engine]() { engine.siguientePaso(); });
        EXPECT_TRUE(esperarHasta(
            [&engine]() { return engine.obtenerEstado().pidEjecutando != -1; }));

        engine.iniciar();  // la rafaga manual sigue en curso
        manual.join();

        EXPECT_TRUE(esperarHasta([&engine]() {
            return engine.obtenerEstado().timeline.size() >= 4U;
        }));
        engine.pausar();

        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::PAUSADO);
        EXPECT_EQ(s.pidEjecutando, -1);
        EXPECT_TRUE(s.timeline.size() >= 4U);
    }

    // Caso 3: pausar() no retorna hasta cerrar la rafaga en curso.
    {
        engine.reiniciar();
        cargarProcesos(engine, 2);
        engine.iniciar();
        EXPECT_TRUE(esperarHasta(
            [&engine]() { return engine.obtenerEstado().pidEjecutando != -1; }));

        engine.pausar();
        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::PAUSADO);
        EXPECT_EQ(s.pidEjecutando, -1);

        // Estable: no hay mas avance tras pausar.
        const std::int64_t reloj = s.tiempoSimulado;
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        EXPECT_EQ(engine.obtenerEstado().tiempoSimulado, reloj);
    }

    // Caso 4: reiniciar() en plena ejecucion continua, incluso desde dos hilos.
    {
        engine.reiniciar();
        cargarProcesos(engine, 4);
        engine.iniciar();
        EXPECT_TRUE(esperarHasta(
            [&engine]() { return engine.obtenerEstado().pidEjecutando != -1; }));

        std::thread r1([&engine]() { engine.reiniciar(); });
        std::thread r2([&engine]() { engine.reiniciar(); });
        r1.join();
        r2.join();

        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::DETENIDO);
        EXPECT_EQ(s.procesos.size(), 0U);
        EXPECT_EQ(s.timeline.size(), 0U);
        EXPECT_EQ(s.eventos.size(), 0U);
        EXPECT_EQ(s.tiempoSimulado, 0);
        EXPECT_EQ(s.pidEjecutando, -1);
        EXPECT_EQ(s.memoriaUsada, 0);
        EXPECT_EQ(engine.cantidadWorkersActivos(), 0);

        // El motor sigue siendo utilizable despues del reinicio.
        cargarProcesos(engine, 1);
        engine.pausar();
        engine.siguientePaso();
        EXPECT_EQ(engine.obtenerEstado().timeline.size(), 1U);
    }

    // Caso 5: ciclos repetidos de iniciar/pausar/reiniciar desde varios hilos.
    {
        engine.reiniciar();
        cargarProcesos(engine, 4);

        std::atomic<bool> seguir{true};
        std::vector<std::thread> controladores;
        controladores.emplace_back([&]() {
            while (seguir.load()) {
                engine.iniciar();
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
                engine.pausar();
            }
        });
        controladores.emplace_back([&]() {
            while (seguir.load()) {
                try {
                    engine.siguientePaso();
                } catch (const std::logic_error&) {
                    // Esperado cuando el motor no esta pausado.
                }
                std::this_thread::yield();
            }
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        seguir.store(false);
        for (auto& hilo : controladores) {
            hilo.join();
        }
        engine.pausar();
        EXPECT_EQ(engine.obtenerEstado().pidEjecutando, -1);
    }

    monitorActivo.store(false);
    monitor.join();
    EXPECT_EQ(violaciones.load(), 0);

    return test::finish();
}
