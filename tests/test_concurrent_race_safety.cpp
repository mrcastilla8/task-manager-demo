#include "concurrent/ConcurrentEngine.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

namespace {

// Devuelve el numero de invariantes violadas por un snapshot. No usa EXPECT_*
// porque se invoca desde varios hilos y los contadores de TestSupport no son
// atomicos.
int invariantesVioladas(const EstadoSistema& s) {
    int violaciones = 0;

    const auto enRun = std::count_if(
        s.procesos.begin(), s.procesos.end(),
        [](const Proceso& p) { return p.estado == Estado::RUN; });
    if (enRun > 1) ++violaciones;  // un unico CPU logico
    if (enRun == 0 && s.pidEjecutando != -1) ++violaciones;
    if (enRun == 1) {
        const auto it = std::find_if(
            s.procesos.begin(), s.procesos.end(),
            [](const Proceso& p) { return p.estado == Estado::RUN; });
        if (it->pid != s.pidEjecutando) ++violaciones;
    }

    std::int64_t memoriaVivos = 0;
    for (const auto& p : s.procesos) {
        if (p.estado != Estado::FINISH && p.estado != Estado::CANCEL) {
            memoriaVivos += p.tamanioBytes;
        }
    }
    if (memoriaVivos != s.memoriaUsada) ++violaciones;
    if (s.memoriaUsada > s.memoriaTotal) ++violaciones;

    std::int64_t finAnterior = 0;
    for (const auto& bloque : s.timeline) {
        if (!bloqueTimelineValido(bloque)) ++violaciones;
        if (bloque.inicio < finAnterior) ++violaciones;  // solape o desorden
        if (bloque.fin > s.tiempoSimulado) ++violaciones;
        finAnterior = bloque.fin;
    }

    std::int64_t tiempoEvento = 0;
    for (const auto& evento : s.eventos) {
        if (evento.tiempoSimulado < tiempoEvento) ++violaciones;
        tiempoEvento = evento.tiempoSimulado;
    }

    // La cola solo contiene procesos READY, sin repetidos y completa.
    const auto listos = std::count_if(
        s.procesos.begin(), s.procesos.end(),
        [](const Proceso& p) { return p.estado == Estado::READY; });
    if (static_cast<std::ptrdiff_t>(s.colaPrioridad.size()) != listos) {
        ++violaciones;
    }
    for (int pid : s.colaPrioridad) {
        const auto it = std::find_if(
            s.procesos.begin(), s.procesos.end(),
            [pid](const Proceso& p) { return p.pid == pid; });
        if (it == s.procesos.end() || it->estado != Estado::READY) {
            ++violaciones;
        }
    }

    return violaciones;
}

}  // namespace

int main() {
    ConcurrentEngine engine(10000);
    engine.configurarIntervaloCoordinadorMs(1);
    engine.configurarQuantum(100);

    // Agregar procesos iniciales
    for (int i = 1; i <= 4; ++i) {
        Proceso p = crearProceso(i, "Base_" + std::to_string(i), 100);
        p.tiempoTotal = 3000;
        p.tiempoRestante = 3000;
        engine.agregarProceso(p);
    }

    std::atomic<bool> ejecutandoPrueba{true};
    std::atomic<int> lecturasExitosas{0};
    std::atomic<int> violaciones{0};

    // Hilos lectores concurrentes que llaman a obtenerEstado() continuamente (simulando GUI / observadores)
    std::vector<std::thread> lectores;
    for (int t = 0; t < 4; ++t) {
        lectores.emplace_back([&engine, &ejecutandoPrueba, &lecturasExitosas,
                               &violaciones]() {
            std::int64_t relojPrevio = 0;
            while (ejecutandoPrueba.load(std::memory_order_relaxed)) {
                EstadoSistema snap = engine.obtenerEstado();
                // Verificacion de consistencia en el snapshot devuelto
                int malas = invariantesVioladas(snap);
                if (snap.tiempoSimulado < relojPrevio) ++malas;  // reloj monotono
                relojPrevio = snap.tiempoSimulado;
                if (malas > 0) {
                    violaciones.fetch_add(malas, std::memory_order_relaxed);
                }
                lecturasExitosas.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::yield();
            }
        });
    }

    // Iniciar el motor en segundo plano
    engine.iniciar();

    // Hilo productor concurrente que agrega procesos en tiempo de ejecucion
    std::thread productor([&engine, &ejecutandoPrueba]() {
        for (int i = 10; i <= 15; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            if (!ejecutandoPrueba.load(std::memory_order_relaxed)) break;
            try {
                Proceso dinamico = crearProceso(i, "Dinamico_" + std::to_string(i), 50);
                dinamico.tiempoTotal = 500;
                dinamico.tiempoRestante = 500;
                engine.agregarProceso(dinamico);
            } catch (...) {
                // Si la memoria se llena o el motor se detiene
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Detener prueba y sincronizar hilos
    ejecutandoPrueba.store(false, std::memory_order_relaxed);

    if (productor.joinable()) {
        productor.join();
    }

    for (auto& lector : lectores) {
        if (lector.joinable()) {
            lector.join();
        }
    }

    engine.pausar();

    EXPECT_TRUE(lecturasExitosas.load() > 100);
    EXPECT_EQ(violaciones.load(), 0);
    EXPECT_EQ(invariantesVioladas(engine.obtenerEstado()), 0);
    EstadoSistema finalSnap = engine.obtenerEstado();
    EXPECT_TRUE(finalSnap.tiempoSimulado > 0);
    EXPECT_TRUE(!finalSnap.timeline.empty());

    return test::finish();
}
