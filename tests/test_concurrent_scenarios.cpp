#include "concurrent/ConcurrentEngine.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

namespace {

std::vector<TipoEvento> tiposDeEventos(const EstadoSistema& estado, int pid) {
    std::vector<TipoEvento> tipos;
    for (const auto& evento : estado.eventos) {
        if (evento.pid == pid) {
            tipos.push_back(evento.tipo);
        }
    }
    return tipos;
}

const Proceso* buscarProceso(const EstadoSistema& estado, int pid) {
    const auto it = std::find_if(
        estado.procesos.begin(), estado.procesos.end(),
        [pid](const Proceso& p) { return p.pid == pid; });
    return it == estado.procesos.end() ? nullptr : &*it;
}

}  // namespace

int main() {
    // Caso 1: ciclo completo con E/S: RUN -> WAIT -> READY -> RUN -> FINISH.
    {
        ConcurrentEngine engine(2048);
        engine.configurarQuantum(500);

        Proceso p = crearProceso(1, "ConIO", 100, Estado::READY, 5, 1);
        p.tiempoTotal = 1000;
        p.tiempoRestante = 1000;
        p.operacionIO = OperacionIO{true, 300, 200};
        engine.agregarProceso(p);
        engine.pausar();

        // Paso 1: ejecuta 300 ms de CPU y se bloquea por E/S.
        engine.siguientePaso();
        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.tiempoSimulado, 300);
        EXPECT_EQ(buscarProceso(s, 1)->estado, Estado::WAIT);
        EXPECT_EQ(s.pidEjecutando, -1);
        EXPECT_TRUE(s.colaPrioridad.empty());
        EXPECT_EQ(s.memoriaUsada, 100);  // WAIT conserva la memoria

        // Paso 2: CPU libre; el reloj salta hasta el fin de la E/S (200 ms).
        engine.siguientePaso();
        s = engine.obtenerEstado();
        EXPECT_EQ(s.tiempoSimulado, 500);
        EXPECT_EQ(buscarProceso(s, 1)->estado, Estado::READY);
        EXPECT_EQ(s.timeline.size(), 1U);  // el tiempo ocioso no genera bloque

        // Pasos 3 y 4: agota quantum y luego termina.
        engine.siguientePaso();
        engine.siguientePaso();
        s = engine.obtenerEstado();
        EXPECT_EQ(buscarProceso(s, 1)->estado, Estado::FINISH);
        EXPECT_EQ(buscarProceso(s, 1)->tiempoRestante, 0);
        EXPECT_EQ(s.memoriaUsada, 0);
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::FINALIZADO);

        EXPECT_EQ(s.timeline.size(), 3U);
        EXPECT_EQ(s.timeline[0].inicio, 0);
        EXPECT_EQ(s.timeline[0].fin, 300);
        EXPECT_EQ(s.timeline[1].inicio, 500);
        EXPECT_EQ(s.timeline[1].fin, 1000);
        EXPECT_EQ(s.timeline[2].inicio, 1000);
        EXPECT_EQ(s.timeline[2].fin, 1200);

        const std::vector<TipoEvento> esperados = {
            TipoEvento::PROCESS_CREATED,  TipoEvento::PROCESS_ADMITTED,
            TipoEvento::PROCESS_READY,    TipoEvento::PROCESS_RUNNING,
            TipoEvento::PROCESS_WAITING,  TipoEvento::PROCESS_READY,
            TipoEvento::PROCESS_RUNNING,  TipoEvento::QUANTUM_FINISHED,
            TipoEvento::PROCESS_READY,    TipoEvento::PROCESS_RUNNING,
            TipoEvento::PROCESS_FINISHED};
        EXPECT_TRUE(tiposDeEventos(s, 1) == esperados);
    }

    // Caso 2 (heredado de v1; DP-10 retira el aging en v2): aging eleva a un
    // proceso de baja prioridad hasta igualar al dominante, y la cola READY del
    // snapshot nunca incluye al proceso en RUN.
    {
        ConcurrentEngine engine(2048);
        engine.configurarQuantum(500);

        Proceso a = crearProceso(1, "Alta", 100, Estado::READY, 8, 1);
        a.tiempoTotal = 20000;
        a.tiempoRestante = 20000;
        Proceso b = crearProceso(2, "Baja", 100, Estado::READY, 3, 2);
        b.tiempoTotal = 2000;
        b.tiempoRestante = 2000;
        engine.agregarProceso(a);
        engine.agregarProceso(b);
        engine.pausar();

        bool bEjecuto = false;
        for (int paso = 0; paso < 100 && !bEjecuto; ++paso) {
            engine.siguientePaso();
            const EstadoSistema s = engine.obtenerEstado();
            bEjecuto = std::any_of(
                s.timeline.begin(), s.timeline.end(),
                [](const BloqueTimeline& bloque) { return bloque.pid == 2; });
        }
        EXPECT_TRUE(bEjecuto);

        const EstadoSistema s = engine.obtenerEstado();
        const auto bloqueB = std::find_if(
            s.timeline.begin(), s.timeline.end(),
            [](const BloqueTimeline& bloque) { return bloque.pid == 2; });
        EXPECT_TRUE(bloqueB != s.timeline.end());
        if (bloqueB != s.timeline.end()) {
            // B espera 5 intervalos de 2,000 ms: 3 -> 8 iguala a A; B gana el
            // empate por tener un ticket mas antiguo (A reingreso tras cada cuota).
            EXPECT_EQ(bloqueB->inicio, 10000);
        }

        const auto cambiosB = std::count_if(
            s.eventos.begin(), s.eventos.end(), [](const Evento& e) {
                return e.pid == 2 && e.tipo == TipoEvento::PRIORITY_CHANGED;
            });
        EXPECT_EQ(cambiosB, 5);

        // Al recibir CPU recupera su prioridad base y sin espera acumulada.
        EXPECT_EQ(buscarProceso(s, 2)->prioridadActual, 3);
        EXPECT_EQ(buscarProceso(s, 2)->tiempoEspera, 0);

        EXPECT_EQ(s.pidEjecutando, -1);
        for (int pid : s.colaPrioridad) {
            EXPECT_EQ(buscarProceso(s, pid)->estado, Estado::READY);
        }
    }

    // Caso 3: ejecucion continua hasta FINALIZADO sin intervencion manual.
    {
        ConcurrentEngine engine(2048);
        engine.configurarIntervaloCoordinadorMs(0);
        engine.configurarQuantum(250);

        std::int64_t sumaTotales = 0;
        for (int i = 1; i <= 3; ++i) {
            Proceso p = crearProceso(i, "P" + std::to_string(i), 100,
                                     Estado::READY, 4 + i, i);
            p.tiempoTotal = 600 + 100 * i;
            p.tiempoRestante = p.tiempoTotal;
            sumaTotales += p.tiempoTotal;
            engine.agregarProceso(p);
        }

        engine.iniciar();
        const auto limite =
            std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (engine.obtenerEstado().estadoEjecucion !=
                   EstadoEjecucion::FINALIZADO &&
               std::chrono::steady_clock::now() < limite) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::FINALIZADO);
        EXPECT_EQ(s.memoriaUsada, 0);
        EXPECT_EQ(s.tiempoSimulado, sumaTotales);  // sin E/S no hay tiempo ocioso

        std::int64_t sumaBloques = 0;
        for (const auto& bloque : s.timeline) {
            sumaBloques += bloque.fin - bloque.inicio;
        }
        EXPECT_EQ(sumaBloques, sumaTotales);
        for (const auto& proceso : s.procesos) {
            EXPECT_EQ(proceso.estado, Estado::FINISH);
        }
    }

    return test::finish();
}
