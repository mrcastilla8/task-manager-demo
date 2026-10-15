#include "concurrent/ConcurrentEngine.h"
#include "tests/Fixtures.h"
#include "tests/SnapshotInvariants.h"
#include "tests/TestSupport.h"

#include <algorithm>
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
        // yield y no sleep: en Windows sleep_for(1ms) puede dormir ~15 ms y la
        // ventana de la rafaga (retardo de pruebas) se perderia.
        std::this_thread::yield();
    }
    return condicion();
}

Proceso crear(int pid, int tamanio, std::int64_t total) {
    Proceso p = crearProceso(pid, "P" + std::to_string(pid), tamanio,
                             Estado::READY, 5, 0);
    p.tiempoTotal = total;
    p.tiempoRestante = total;
    return p;
}

const Proceso* buscar(const EstadoSistema& estado, int pid) {
    const auto it = std::find_if(
        estado.procesos.begin(), estado.procesos.end(),
        [pid](const Proceso& p) { return p.pid == pid; });
    return it == estado.procesos.end() ? nullptr : &*it;
}

std::vector<TipoEvento> tipos(const EstadoSistema& estado, int pid) {
    std::vector<TipoEvento> resultado;
    for (const auto& evento : estado.eventos) {
        if (evento.pid == pid) {
            resultado.push_back(evento.tipo);
        }
    }
    return resultado;
}

long cancelaciones(const EstadoSistema& estado, int pid) {
    return std::count_if(
        estado.eventos.begin(), estado.eventos.end(), [pid](const Evento& e) {
            return e.pid == pid && e.tipo == TipoEvento::PROCESS_CANCELLED;
        });
}

// Ejecuta un siguientePaso() en otro hilo y espera a que el proceso `pid` este
// en RUN para que el llamante pueda cancelar con la rafaga en curso.
struct PasoEnCurso {
    std::thread hilo;
    PasoEnCurso(ConcurrentEngine& engine, int pid) {
        hilo = std::thread([&engine]() { engine.siguientePaso(); });
        enRun = esperarHasta([&engine, pid]() {
            return engine.obtenerEstado().pidEjecutando == pid;
        });
    }
    void terminar() { hilo.join(); }
    bool enRun{false};
};

}  // namespace

int main() {
    // Caso 1: cancelar en RUN conserva el CPU completado, libera la memoria una
    // sola vez y emite un unico evento al cerrar la rafaga.
    {
        ConcurrentEngine engine(2048);
        engine.configurarQuantum(500);
        engine.configurarRetardoRafagaPruebasMs(40);
        engine.agregarProceso(crear(1, 400, 2000));
        engine.agregarProceso(crear(2, 100, 300));
        engine.pausar();

        PasoEnCurso paso(engine, 1);
        EXPECT_TRUE(paso.enRun);
        engine.cancelarProceso(1);
        engine.cancelarProceso(1);  // idempotente durante la rafaga
        paso.terminar();

        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->estado, Estado::CANCEL);
        EXPECT_EQ(buscar(s, 1)->tiempoRestante, 1500);  // 500 ms completados
        EXPECT_EQ(s.timeline.size(), 1U);
        EXPECT_EQ(s.timeline[0].pid, 1);
        EXPECT_EQ(s.timeline[0].inicio, 0);
        EXPECT_EQ(s.timeline[0].fin, 500);
        EXPECT_EQ(s.tiempoSimulado, 500);
        EXPECT_EQ(s.memoriaUsada, 100);  // solo queda P2
        EXPECT_EQ(s.pidEjecutando, -1);
        EXPECT_EQ(cancelaciones(s, 1), 1);
        EXPECT_EQ(s.eventos.back().tipo, TipoEvento::PROCESS_CANCELLED);
        EXPECT_EQ(s.eventos.back().tiempoSimulado, 500);
        EXPECT_EQ(engine.cantidadWorkersActivos(), 1);
        EXPECT_EQ(invariantesVioladas(s), 0);

        // El siguiente paso despacha al proceso restante.
        engine.siguientePaso();
        s = engine.obtenerEstado();
        EXPECT_EQ(s.timeline.size(), 2U);
        EXPECT_EQ(s.timeline[1].pid, 2);
        EXPECT_EQ(buscar(s, 2)->estado, Estado::FINISH);
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::FINALIZADO);
        EXPECT_EQ(s.memoriaUsada, 0);
    }

    // Caso 2: si el proceso termina en el mismo limite, FINISH gana y la
    // cancelacion no genera evento ni libera la memoria dos veces.
    {
        ConcurrentEngine engine(2048);
        engine.configurarQuantum(500);
        engine.configurarRetardoRafagaPruebasMs(40);
        engine.agregarProceso(crear(1, 400, 400));
        engine.pausar();

        PasoEnCurso paso(engine, 1);
        EXPECT_TRUE(paso.enRun);
        engine.cancelarProceso(1);
        paso.terminar();

        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->estado, Estado::FINISH);
        EXPECT_EQ(cancelaciones(s, 1), 0);
        EXPECT_EQ(s.memoriaUsada, 0);
        EXPECT_EQ(s.timeline.size(), 1U);
        EXPECT_EQ(s.timeline[0].fin, 400);

        // Cancelar de nuevo un proceso ya terminado sigue siendo un no-op.
        engine.cancelarProceso(1);
        EXPECT_EQ(engine.obtenerEstado().eventos.size(), s.eventos.size());
    }

    // Caso 3: la rafaga termina en E/S (WAIT) y despues se aplica la cancelacion.
    {
        ConcurrentEngine engine(2048);
        engine.configurarQuantum(500);
        engine.configurarRetardoRafagaPruebasMs(40);
        Proceso p = crear(1, 400, 2000);
        p.operacionIO = OperacionIO{true, 200, 300};
        engine.agregarProceso(p);
        engine.pausar();

        PasoEnCurso paso(engine, 1);
        EXPECT_TRUE(paso.enRun);
        engine.cancelarProceso(1);
        paso.terminar();

        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->estado, Estado::CANCEL);
        EXPECT_EQ(s.timeline.size(), 1U);
        EXPECT_EQ(s.timeline[0].fin, 200);
        EXPECT_EQ(cancelaciones(s, 1), 1);
        EXPECT_EQ(s.memoriaUsada, 0);
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::FINALIZADO);

        const std::vector<TipoEvento> esperado = {
            TipoEvento::PROCESS_CREATED, TipoEvento::PROCESS_ADMITTED,
            TipoEvento::PROCESS_READY,   TipoEvento::PROCESS_RUNNING,
            TipoEvento::PROCESS_WAITING, TipoEvento::PROCESS_CANCELLED};
        EXPECT_TRUE(tipos(s, 1) == esperado);
    }

    // Caso 4: reiniciar durante la rafaga espera a que cierre y no deja una
    // cancelacion pendiente que afecte a una sesion nueva con el mismo PID.
    {
        ConcurrentEngine engine(2048);
        engine.configurarQuantum(500);
        engine.configurarRetardoRafagaPruebasMs(40);
        engine.agregarProceso(crear(1, 400, 2000));
        engine.pausar();

        PasoEnCurso paso(engine, 1);
        EXPECT_TRUE(paso.enRun);
        engine.cancelarProceso(1);
        std::thread reinicio([&engine]() { engine.reiniciar(); });
        paso.terminar();
        reinicio.join();

        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.procesos.size(), 0U);
        EXPECT_EQ(s.timeline.size(), 0U);
        EXPECT_EQ(engine.cantidadWorkersActivos(), 0);

        engine.agregarProceso(crear(1, 400, 2000));
        engine.pausar();
        engine.siguientePaso();
        s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->estado, Estado::READY);
        EXPECT_EQ(s.timeline.size(), 1U);
    }

    // Caso 5: cancelar en RUN durante la ejecucion continua.
    {
        ConcurrentEngine engine(2048);
        engine.configurarIntervaloCoordinadorMs(0);
        engine.configurarQuantum(100);
        engine.configurarRetardoRafagaPruebasMs(30);
        engine.agregarProceso(crear(1, 400, 1000));
        engine.agregarProceso(crear(2, 100, 600));

        engine.iniciar();
        EXPECT_TRUE(esperarHasta([&engine]() {
            return engine.obtenerEstado().pidEjecutando == 1;
        }));
        engine.cancelarProceso(1);
        EXPECT_TRUE(esperarHasta([&engine]() {
            return engine.obtenerEstado().estadoEjecucion ==
                   EstadoEjecucion::FINALIZADO;
        }));

        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->estado, Estado::CANCEL);
        EXPECT_EQ(buscar(s, 2)->estado, Estado::FINISH);
        EXPECT_EQ(cancelaciones(s, 1), 1);
        EXPECT_EQ(s.memoriaUsada, 0);
        EXPECT_EQ(invariantesVioladas(s), 0);

        // El CPU completado de P1 antes de la cancelacion consta en el timeline.
        const std::int64_t cpuP1 = 1000 - buscar(s, 1)->tiempoRestante;
        std::int64_t sumaBloquesP1 = 0;
        for (const auto& bloque : s.timeline) {
            if (bloque.pid == 1) {
                sumaBloquesP1 += bloque.fin - bloque.inicio;
            }
        }
        EXPECT_EQ(sumaBloquesP1, cpuP1);
        EXPECT_TRUE(cpuP1 > 0);
    }

    return test::finish();
}
