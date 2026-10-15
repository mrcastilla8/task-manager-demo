#include "concurrent/ConcurrentEngine.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

#include <algorithm>
#include <set>
#include <vector>

namespace {

Proceso crear(int pid, int prioridad, std::int64_t total) {
    Proceso p = crearProceso(pid, "P" + std::to_string(pid), 100, Estado::READY,
                             prioridad, 0);
    p.tiempoTotal = total;
    p.tiempoRestante = total;
    return p;
}

std::vector<int> pidsDelTimeline(const EstadoSistema& estado) {
    std::vector<int> pids;
    for (const auto& bloque : estado.timeline) {
        pids.push_back(bloque.pid);
    }
    return pids;
}

const Proceso* buscar(const EstadoSistema& estado, int pid) {
    const auto it = std::find_if(
        estado.procesos.begin(), estado.procesos.end(),
        [pid](const Proceso& p) { return p.pid == pid; });
    return it == estado.procesos.end() ? nullptr : &*it;
}

}  // namespace

int main() {
    // Caso 1: dos procesos con la misma prioridad alternan por turnos.
    {
        ConcurrentEngine engine(2048);
        engine.configurarQuantum(250);
        engine.agregarProceso(crear(1, 8, 1000));
        engine.agregarProceso(crear(2, 8, 1000));
        engine.pausar();

        for (int i = 0; i < 8; ++i) {
            engine.siguientePaso();
        }

        const EstadoSistema s = engine.obtenerEstado();
        const std::vector<int> esperado = {1, 2, 1, 2, 1, 2, 1, 2};
        EXPECT_TRUE(pidsDelTimeline(s) == esperado);
        EXPECT_EQ(s.tiempoSimulado, 2000);
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::FINALIZADO);
        for (const auto& bloque : s.timeline) {
            EXPECT_EQ(bloque.fin - bloque.inicio, 250);
        }
    }

    // Caso 2: el ticket de alta lo asigna el motor (ignora el valor recibido) y
    // el reingreso por fin de cuota recibe un ticket mayor que sus pares.
    {
        ConcurrentEngine engine(2048);
        engine.configurarQuantum(100);
        Proceso a = crear(1, 5, 1000);
        a.ordenLlegada = 99;
        Proceso b = crear(2, 5, 1000);
        b.ordenLlegada = 0;
        Proceso c = crear(3, 5, 1000);
        c.ordenLlegada = 0;
        engine.agregarProceso(a);
        engine.agregarProceso(b);
        engine.agregarProceso(c);

        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->ordenLlegada, 1);
        EXPECT_EQ(buscar(s, 2)->ordenLlegada, 2);
        EXPECT_EQ(buscar(s, 3)->ordenLlegada, 3);

        engine.pausar();
        engine.siguientePaso();  // A agota cuota y reingresa
        s = engine.obtenerEstado();
        EXPECT_TRUE(buscar(s, 1)->ordenLlegada > buscar(s, 3)->ordenLlegada);
        const std::vector<int> colaEsperada = {2, 3, 1};
        EXPECT_TRUE(s.colaPrioridad == colaEsperada);

        // Los tickets son unicos mientras avanza la simulacion.
        for (int i = 0; i < 6; ++i) {
            engine.siguientePaso();
        }
        s = engine.obtenerEstado();
        std::set<std::int64_t> tickets;
        for (const auto& proceso : s.procesos) {
            tickets.insert(proceso.ordenLlegada);
        }
        EXPECT_EQ(tickets.size(), s.procesos.size());
    }

    // Caso 3: un proceso que vuelve de WAIT queda detras de sus pares segun el
    // instante en que despierta.
    {
        ConcurrentEngine engine(2048);
        engine.configurarQuantum(500);
        Proceso x = crear(1, 5, 2000);
        x.operacionIO = OperacionIO{true, 100, 300};
        engine.agregarProceso(x);
        engine.agregarProceso(crear(2, 5, 2000));
        engine.agregarProceso(crear(3, 5, 2000));
        engine.pausar();

        for (int i = 0; i < 6; ++i) {
            engine.siguientePaso();
        }

        // X: 100 ms y a WAIT; despierta (t=400) durante el turno de Y, es decir
        // antes de que Y agote su cuota, y por eso reingresa antes que Y.
        const std::vector<int> esperado = {1, 2, 3, 1, 2, 3};
        EXPECT_TRUE(pidsDelTimeline(engine.obtenerEstado()) == esperado);
    }

    // Caso 4: varios procesos que despiertan en el mismo turno reingresan de
    // forma determinista: antes el que despierta antes, luego ticket de bloqueo.
    {
        ConcurrentEngine engine(2048);
        engine.configurarQuantum(500);
        Proceso p1 = crear(1, 5, 1000);
        p1.operacionIO = OperacionIO{true, 100, 300};
        Proceso p2 = crear(2, 5, 1000);
        p2.operacionIO = OperacionIO{true, 100, 300};
        engine.agregarProceso(p1);
        engine.agregarProceso(p2);
        engine.agregarProceso(crear(3, 5, 4000));
        engine.pausar();

        for (int i = 0; i < 6; ++i) {
            engine.siguientePaso();
        }

        // P1 y P2 se bloquean tras 100 ms; despiertan durante el turno de P3.
        // P1 se bloqueo antes y por tanto despierta antes que P2.
        const std::vector<int> esperado = {1, 2, 3, 1, 2, 3};
        EXPECT_TRUE(pidsDelTimeline(engine.obtenerEstado()) == esperado);
    }

    return test::finish();
}
