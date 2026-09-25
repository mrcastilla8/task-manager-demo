#include "concurrent/ConcurrentEngine.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

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

    // Hilos lectores concurrentes que llaman a obtenerEstado() continuamente (simulando GUI / observadores)
    std::vector<std::thread> lectores;
    for (int t = 0; t < 4; ++t) {
        lectores.emplace_back([&engine, &ejecutandoPrueba, &lecturasExitosas]() {
            while (ejecutandoPrueba.load(std::memory_order_relaxed)) {
                EstadoSistema snap = engine.obtenerEstado();
                // Verificacion de consistencia en el snapshot devuelto
                EXPECT_TRUE(snap.memoriaUsada >= 0);
                EXPECT_TRUE(snap.memoriaTotal > 0);
                EXPECT_TRUE(snap.tiempoSimulado >= 0);
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
    EstadoSistema finalSnap = engine.obtenerEstado();
    EXPECT_TRUE(finalSnap.tiempoSimulado > 0);
    EXPECT_TRUE(!finalSnap.timeline.empty());

    return test::finish();
}
