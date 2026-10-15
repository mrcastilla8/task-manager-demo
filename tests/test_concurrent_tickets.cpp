#include "concurrent/ConcurrentEngine.h"
#include "tests/Fixtures.h"
#include "tests/SnapshotInvariants.h"
#include "tests/TestSupport.h"

#include <chrono>
#include <limits>
#include <stdexcept>
#include <thread>

namespace {

constexpr std::int64_t MAXIMO = std::numeric_limits<std::int64_t>::max();

Proceso crear(int pid, std::int64_t total) {
    Proceso p = crearProceso(pid, "P" + std::to_string(pid), 100, Estado::READY,
                             5, 0);
    p.tiempoTotal = total;
    p.tiempoRestante = total;
    return p;
}

bool igualesSnapshot(const EstadoSistema& a, const EstadoSistema& b) {
    if (a.tiempoSimulado != b.tiempoSimulado ||
        a.timeline.size() != b.timeline.size() ||
        a.eventos.size() != b.eventos.size() ||
        a.procesos.size() != b.procesos.size() ||
        a.colaPrioridad != b.colaPrioridad || a.memoriaUsada != b.memoriaUsada) {
        return false;
    }
    for (std::size_t i = 0; i < a.procesos.size(); ++i) {
        if (a.procesos[i].estado != b.procesos[i].estado ||
            a.procesos[i].ordenLlegada != b.procesos[i].ordenLlegada ||
            a.procesos[i].tiempoRestante != b.procesos[i].tiempoRestante) {
            return false;
        }
    }
    return true;
}

}  // namespace

int main() {
    // Caso 1: el alta falla sin efectos cuando no quedan tickets.
    {
        ConcurrentEngine engine(2048);
        engine.fijarSiguienteTicketPruebas(MAXIMO);
        EXPECT_THROW(engine.agregarProceso(crear(1, 500)), std::overflow_error);
        EXPECT_THROW(engine.agregarProceso(crear(1, 500)), std::runtime_error);

        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.procesos.size(), 0U);
        EXPECT_EQ(s.eventos.size(), 0U);
        EXPECT_EQ(s.memoriaUsada, 0);
        EXPECT_EQ(engine.cantidadWorkersActivos(), 0);

        // Queda exactamente un ticket utilizable (MAXIMO - 1).
        engine.fijarSiguienteTicketPruebas(MAXIMO - 1);
        engine.agregarProceso(crear(1, 500));
        EXPECT_EQ(engine.obtenerEstado().procesos[0].ordenLlegada, MAXIMO - 1);
        EXPECT_THROW(engine.agregarProceso(crear(2, 500)), std::overflow_error);
        EXPECT_EQ(engine.obtenerEstado().procesos.size(), 1U);
        EXPECT_EQ(engine.memoriaUsada(), 100);
        EXPECT_EQ(engine.cantidadWorkersActivos(), 1);
    }

    // Caso 2: un paso que necesita un ticket y no lo tiene lanza sin cambiar
    // nada; el motor sigue siendo utilizable.
    {
        ConcurrentEngine engine(2048);
        engine.configurarQuantum(100);
        engine.agregarProceso(crear(1, 1000));
        engine.agregarProceso(crear(2, 1000));
        engine.fijarSiguienteTicketPruebas(MAXIMO - 1);
        engine.pausar();

        engine.siguientePaso();  // ultimo ticket utilizable
        const EstadoSistema antes = engine.obtenerEstado();
        EXPECT_EQ(antes.timeline.size(), 1U);

        EXPECT_THROW(engine.siguientePaso(), std::overflow_error);
        const EstadoSistema despues = engine.obtenerEstado();
        EXPECT_TRUE(igualesSnapshot(antes, despues));
        EXPECT_EQ(despues.estadoEjecucion, EstadoEjecucion::PAUSADO);
        EXPECT_EQ(despues.pidEjecutando, -1);
        EXPECT_EQ(invariantesVioladas(despues), 0);

        // Reiniciar reinicia el contador y el motor vuelve a funcionar.
        engine.reiniciar();
        engine.agregarProceso(crear(1, 300));
        EXPECT_EQ(engine.obtenerEstado().procesos[0].ordenLlegada, 1);
        engine.pausar();
        engine.siguientePaso();
        EXPECT_EQ(engine.obtenerEstado().timeline.size(), 1U);
    }

    // Caso 3: la comprobacion cuenta los despertares de WAIT. Con un solo ticket
    // libre, un paso que necesitaria dos (fin de cuota de P2 y despertar de P1)
    // se rechaza completo, sin dejar el paso a medias.
    {
        ConcurrentEngine engine(2048);
        engine.configurarQuantum(500);
        Proceso p1 = crear(1, 2000);
        p1.operacionIO = OperacionIO{true, 100, 300};
        engine.agregarProceso(p1);
        engine.agregarProceso(crear(2, 2000));
        engine.fijarSiguienteTicketPruebas(MAXIMO - 2);
        engine.pausar();

        engine.siguientePaso();  // P1 se bloquea (ticket MAXIMO - 2)
        const EstadoSistema antes = engine.obtenerEstado();
        EXPECT_EQ(antes.procesos[0].estado, Estado::WAIT);

        EXPECT_THROW(engine.siguientePaso(), std::overflow_error);
        const EstadoSistema despues = engine.obtenerEstado();
        EXPECT_TRUE(igualesSnapshot(antes, despues));
        EXPECT_EQ(despues.procesos[0].estado, Estado::WAIT);
        EXPECT_EQ(despues.procesos[1].estado, Estado::READY);
        EXPECT_EQ(invariantesVioladas(despues), 0);
    }

    // Caso 4: en ejecucion continua el coordinador no termina ni se bloquea: al
    // agotar los tickets el motor queda PAUSADO y reiniciar lo recupera.
    {
        ConcurrentEngine engine(2048);
        engine.configurarIntervaloCoordinadorMs(0);
        engine.configurarQuantum(100);
        engine.agregarProceso(crear(1, 5000));
        engine.agregarProceso(crear(2, 5000));
        engine.fijarSiguienteTicketPruebas(MAXIMO - 1);

        engine.iniciar();
        const auto limite =
            std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (engine.obtenerEstado().estadoEjecucion !=
                   EstadoEjecucion::PAUSADO &&
               std::chrono::steady_clock::now() < limite) {
            std::this_thread::yield();
        }

        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::PAUSADO);
        EXPECT_EQ(s.timeline.size(), 1U);
        EXPECT_EQ(s.pidEjecutando, -1);
        EXPECT_EQ(invariantesVioladas(s), 0);

        engine.reiniciar();
        engine.agregarProceso(crear(1, 300));
        engine.iniciar();
        const auto limite2 =
            std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (engine.obtenerEstado().estadoEjecucion !=
                   EstadoEjecucion::FINALIZADO &&
               std::chrono::steady_clock::now() < limite2) {
            std::this_thread::yield();
        }
        EXPECT_EQ(engine.obtenerEstado().estadoEjecucion,
                  EstadoEjecucion::FINALIZADO);
    }

    return test::finish();
}
