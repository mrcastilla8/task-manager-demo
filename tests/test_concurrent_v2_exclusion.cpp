// Exclusion del CPU logico y consistencia de snapshots bajo concurrencia: un solo
// proceso en RUN, pasos serializados y lectores simultaneos sin estados rotos.
#include "tests/ConcurrentV2Support.h"
#include "tests/TestSupport.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace ConcurrentSimulator::v2;
using namespace ConcurrentSimulator::v2::testing;

int main() {
    // Caso 1: dos hilos llaman a siguientePaso() a la vez. El coordinador los
    // serializa: mientras uno esta en RUN el otro espera y nunca hay dos en RUN.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        engine.activarPuertaRafagaPruebas(true);
        EXPECT_TRUE(engine.agregarProceso(alta(1, 50, 200)));
        EXPECT_TRUE(engine.agregarProceso(alta(2, 50, 200)));
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());

        std::atomic<bool> segundoTermino{false};
        ResultadoPaso segundo;
        std::thread t2;
        {
            PasoRetenido primero(engine);
            EXPECT_TRUE(primero.enRun());
            t2 = std::thread([&]() {
                segundo = engine.siguientePaso();
                segundoTermino = true;
            });
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            const EstadoSistema durante = engine.obtenerEstado();
            EXPECT_FALSE(segundoTermino.load());
            EXPECT_EQ(durante.timeline.size(), 0U);  // la primera rafaga sigue
            EXPECT_EQ(invariantesVioladas(durante), 0);
            primero.terminar();
        }
        // El segundo paso arranca solo cuando cierra el primero.
        EXPECT_TRUE(esperarHasta([&engine]() {
            const EstadoSistema s = engine.obtenerEstado();
            return s.pidEjecutando != -1 && s.timeline.size() == 1U;
        }));
        EXPECT_EQ(invariantesVioladas(engine.obtenerEstado()), 0);
        engine.liberarRafagaPruebas();
        t2.join();
        EXPECT_TRUE(segundoTermino.load());
        EXPECT_EQ(segundo.codigo, CodigoResultado::OK);

        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.timeline.size(), 2U);
        EXPECT_EQ(s.timeline[0].finMs(), s.timeline[1].inicioMs());  // sin solape
        EXPECT_EQ(s.tiempoSimuladoMs, 60);
        engine.activarPuertaRafagaPruebas(false);
    }

    // Caso 2: lectores simultaneos con un hilo que avanza el motor. Todos los
    // snapshots cumplen las invariantes y el reloj nunca retrocede.
    {
        ConcurrentEngineV2 engine(Configuracion{100000});
        for (int pid = 1; pid <= 5; ++pid) {
            EXPECT_TRUE(engine.agregarProceso(alta(pid, 20 + 7 * pid, 600, {true, 25, 20})));
        }
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());

        std::atomic<bool> activo{true};
        std::atomic<int> violaciones{0};
        std::atomic<int> lecturas{0};
        std::vector<std::thread> lectores;
        for (int i = 0; i < 4; ++i) {
            lectores.emplace_back([&]() {
                std::int64_t relojPrevio = 0;
                while (activo.load()) {
                    const EstadoSistema s = engine.obtenerEstado();
                    int malas = invariantesVioladas(s);
                    if (!prioridadesCoherentes(s)) ++malas;
                    if (s.tiempoSimuladoMs < relojPrevio) ++malas;
                    relojPrevio = s.tiempoSimuladoMs;
                    if (malas > 0) violaciones.fetch_add(malas);
                    lecturas.fetch_add(1);
                    std::this_thread::yield();
                }
            });
        }
        int pasos = 0;
        while (engine.obtenerEstado().estadoEjecucion !=
                   EstadoEjecucion::FINALIZADO &&
               pasos < 2000) {
            engine.siguientePaso();
            ++pasos;
        }
        activo = false;
        for (std::thread& lector : lectores) lector.join();

        EXPECT_EQ(violaciones.load(), 0);
        EXPECT_TRUE(lecturas.load() > 50);
        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::FINALIZADO);
        EXPECT_EQ(s.memoriaUsadaBytes, 0);
    }

    // Caso 3: estres. Pasos, altas, cancelaciones, iniciar/pausar y lecturas
    // simultaneos durante un tiempo acotado; ninguna invariante se rompe, nada se
    // bloquea y todos los hilos terminan.
    {
        ConcurrentEngineV2 engine(Configuracion{200000});
        std::atomic<bool> activo{true};
        std::atomic<int> violaciones{0};
        std::atomic<int> siguientePid{1};
        std::vector<std::thread> hilos;

        for (int i = 0; i < 2; ++i) {  // dos hilos que avanzan el motor
            hilos.emplace_back([&]() {
                while (activo.load()) {
                    engine.iniciar();
                    engine.pausar();
                    engine.siguientePaso();
                }
            });
        }
        hilos.emplace_back([&]() {  // altas continuas
            while (activo.load()) {
                const int pid = siguientePid.fetch_add(1);
                engine.agregarProceso(alta(pid, 10 + (pid * 37) % 200,
                                           20 + (pid * 11) % 80,
                                           pid % 3 == 0 ? OperacionIO{true, 5, 15}
                                                        : OperacionIO{}));
                std::this_thread::yield();
            }
        });
        hilos.emplace_back([&]() {  // cancelaciones
            int pid = 1;
            while (activo.load()) {
                engine.cancelarProceso(pid);
                pid = pid % 60 + 1;
                std::this_thread::yield();
            }
        });
        for (int i = 0; i < 3; ++i) {  // lectores
            hilos.emplace_back([&]() {
                while (activo.load()) {
                    const EstadoSistema s = engine.obtenerEstado();
                    int malas = invariantesVioladas(s);
                    if (!prioridadesCoherentes(s)) ++malas;
                    if (malas > 0) violaciones.fetch_add(malas);
                    std::this_thread::yield();
                }
            });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        activo = false;
        for (std::thread& hilo : hilos) hilo.join();

        EXPECT_EQ(violaciones.load(), 0);
        engine.pausar();
        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.pidEjecutando, -1);
        EXPECT_EQ(invariantesVioladas(s), 0);
        EXPECT_TRUE(s.procesos.size() > 5);  // el motor avanzo de verdad
        EXPECT_TRUE(s.tiempoSimuladoMs > 0);
    }

    return test::finish();
}
