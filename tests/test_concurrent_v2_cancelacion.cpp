// Cancelacion en el motor concurrente v2 (DP-06). Cancelar durante RUN conserva
// el CPU realmente completado; el protocolo de checkpoints de la cancelacion
// parcial sigue pendiente de G1 y no se implementa aqui.
#include "tests/ConcurrentV2Support.h"
#include "tests/TestSupport.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace ConcurrentSimulator::v2;
using namespace ConcurrentSimulator::v2::testing;

namespace {

void prepararPaso(ConcurrentEngineV2& engine) {
    EXPECT_TRUE(engine.iniciar());
    EXPECT_TRUE(engine.pausar());
}

std::size_t indiceEvento(const EstadoSistema& s, TipoEvento tipo, int pid) {
    for (std::size_t i = 0; i < s.eventos.size(); ++i) {
        if (s.eventos[i].tipo == tipo && s.eventos[i].pid == pid) return i;
    }
    return s.eventos.size();
}

}  // namespace

int main() {
    // Caso 1: codigos, READY, WAIT y terminales.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 96, 100, {true, 10, 500})));  // X
        EXPECT_TRUE(engine.agregarProceso(alta(2, 40, 100)));                   // Y
        EXPECT_TRUE(engine.agregarProceso(alta(3, 12, 100)));                   // Z

        EXPECT_EQ(engine.cancelarProceso(99).codigo, CodigoResultado::PID_INEXISTENTE);

        // READY: libera memoria y worker, un solo evento, idempotente.
        EXPECT_EQ(engine.cancelarProceso(3).codigo, CodigoResultado::OK);
        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 3)->estado, Estado::CANCEL);
        EXPECT_EQ(s.memoriaUsadaBytes, 96 + 40);
        EXPECT_EQ(engine.cantidadWorkers(), 2U);
        EXPECT_EQ(contarEventos(s, TipoEvento::PROCESO_CANCELADO, 3), 1);
        EXPECT_TRUE(prioridadesCoherentes(s));
        const std::size_t eventosAntes = s.eventos.size();
        EXPECT_EQ(engine.cancelarProceso(3).codigo, CodigoResultado::PID_YA_CANCELADO);
        EXPECT_EQ(engine.obtenerEstado().eventos.size(), eventosAntes);

        // WAIT: X corre 10 ms y pasa a WAIT; cancelarlo evita su despertar.
        prepararPaso(engine);
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
        EXPECT_EQ(buscar(engine.obtenerEstado(), 1)->estado, Estado::WAIT);
        EXPECT_EQ(engine.cancelarProceso(1).codigo, CodigoResultado::OK);
        s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->estado, Estado::CANCEL);
        EXPECT_EQ(s.memoriaUsadaBytes, 40);
        EXPECT_EQ(engine.cantidadWorkers(), 1U);
        EXPECT_TRUE(indiceEvento(s, TipoEvento::PROCESO_CANCELADO, 1) >
                    indiceEvento(s, TipoEvento::RUN_A_WAIT, 1));

        // El proceso restante termina sin ningun despertar de X.
        while (engine.obtenerEstado().estadoEjecucion !=
               EstadoEjecucion::FINALIZADO) {
            EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
        }
        s = engine.obtenerEstado();
        EXPECT_EQ(contarEventos(s, TipoEvento::WAIT_A_READY, 1), 0);
        EXPECT_EQ(contarEventos(s, TipoEvento::CPU_IDLE_INICIO), 0);
        EXPECT_EQ(buscar(s, 2)->estado, Estado::FINISH);
        EXPECT_EQ(engine.cancelarProceso(2).codigo, CodigoResultado::PID_FINALIZADO);
        EXPECT_EQ(engine.cancelarProceso(1).codigo, CodigoResultado::PID_YA_CANCELADO);
        EXPECT_EQ(contarEventos(s, TipoEvento::SISTEMA_FINALIZADO), 1);
    }

    // Caso 2: cancelar con el proceso en RUN. La cancelacion es inmediata para el
    // llamador (no espera), el CPU completado se conserva y el evento es unico.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        engine.activarPuertaRafagaPruebas(true);
        EXPECT_TRUE(engine.agregarProceso(alta(1, 96, 100)));
        EXPECT_TRUE(engine.agregarProceso(alta(2, 40, 100)));
        prepararPaso(engine);

        {
            PasoRetenido paso(engine, 1);
            EXPECT_TRUE(paso.enRun());
            EXPECT_EQ(engine.cancelarProceso(1).codigo, CodigoResultado::OK);
            EXPECT_EQ(engine.cancelarProceso(1).codigo, CodigoResultado::OK);
            const EstadoSistema durante = engine.obtenerEstado();
            EXPECT_EQ(buscar(durante, 1)->estado, Estado::RUN);  // aun en curso
            EXPECT_EQ(contarEventos(durante, TipoEvento::PROCESO_CANCELADO), 0);
            EXPECT_EQ(invariantesVioladas(durante), 0);
            EXPECT_EQ(paso.terminar().tiempoAvanzadoMs, 30);
        }
        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->estado, Estado::CANCEL);
        EXPECT_EQ(buscar(s, 1)->tiempoCpuConsumidoMs, 30);  // CPU conservado
        EXPECT_EQ(buscar(s, 1)->tiempoCpuRestanteMs, 70);
        EXPECT_EQ(s.timeline.size(), 1U);
        EXPECT_EQ(s.timeline[0].pid(), 1);
        EXPECT_EQ(s.timeline[0].finMs(), 30);
        EXPECT_EQ(s.tiempoSimuladoMs, 30);
        EXPECT_EQ(s.memoriaUsadaBytes, 40);  // memoria liberada una sola vez
        EXPECT_EQ(contarEventos(s, TipoEvento::PROCESO_CANCELADO, 1), 1);
        EXPECT_TRUE(indiceEvento(s, TipoEvento::PROCESO_CANCELADO, 1) >
                    indiceEvento(s, TipoEvento::CPU_EJECUTADA, 1));
        EXPECT_EQ(s.pidEjecutando, -1);
        EXPECT_EQ(engine.cantidadWorkers(), 1U);
        EXPECT_EQ(invariantesVioladas(s), 0);
        EXPECT_TRUE(prioridadesCoherentes(s));

        // El siguiente paso despacha al proceso restante.
        {
            PasoRetenido paso(engine, 2);
            EXPECT_TRUE(paso.enRun());
            EXPECT_EQ(paso.terminar().tiempoAvanzadoMs, 30);
        }
        EXPECT_EQ(engine.obtenerEstado().timeline.back().pid(), 2);
        engine.activarPuertaRafagaPruebas(false);
    }

    // Caso 3: si el proceso termina en ese mismo limite, FINISH gana.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        engine.activarPuertaRafagaPruebas(true);
        EXPECT_TRUE(engine.agregarProceso(alta(1, 96, 20)));
        prepararPaso(engine);
        {
            PasoRetenido paso(engine, 1);
            EXPECT_TRUE(paso.enRun());
            EXPECT_EQ(engine.cancelarProceso(1).codigo, CodigoResultado::OK);
            EXPECT_EQ(paso.terminar().tiempoAvanzadoMs, 20);
        }
        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->estado, Estado::FINISH);
        EXPECT_EQ(contarEventos(s, TipoEvento::PROCESO_CANCELADO), 0);
        EXPECT_EQ(s.memoriaUsadaBytes, 0);
        EXPECT_EQ(engine.cancelarProceso(1).codigo, CodigoResultado::PID_FINALIZADO);
        engine.activarPuertaRafagaPruebas(false);
    }

    // Caso 4: la rafaga termina en WAIT y despues se aplica la cancelacion.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        engine.activarPuertaRafagaPruebas(true);
        EXPECT_TRUE(engine.agregarProceso(alta(1, 96, 100, {true, 8, 50})));
        prepararPaso(engine);
        {
            PasoRetenido paso(engine, 1);
            EXPECT_TRUE(paso.enRun());
            EXPECT_EQ(engine.cancelarProceso(1).codigo, CodigoResultado::OK);
            EXPECT_EQ(paso.terminar().tiempoAvanzadoMs, 8);
        }
        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->estado, Estado::CANCEL);
        EXPECT_EQ(s.timeline.size(), 1U);
        EXPECT_EQ(s.timeline[0].finMs(), 8);
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::FINALIZADO);
        EXPECT_EQ(contarEventos(s, TipoEvento::SISTEMA_FINALIZADO), 1);
        EXPECT_EQ(contarEventos(s, TipoEvento::WAIT_A_READY), 0);
        EXPECT_EQ(s.memoriaUsadaBytes, 0);
        EXPECT_EQ(engine.cantidadWorkers(), 0U);
        engine.activarPuertaRafagaPruebas(false);
    }

    // Caso 5: reiniciar durante la rafaga espera a que cierre y no deja una
    // cancelacion pendiente que afecte a una sesion nueva con el mismo PID.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        engine.activarPuertaRafagaPruebas(true);
        EXPECT_TRUE(engine.agregarProceso(alta(1, 96, 100)));
        prepararPaso(engine);

        std::atomic<bool> reiniciado{false};
        std::thread reinicio;
        {
            PasoRetenido paso(engine, 1);
            EXPECT_TRUE(paso.enRun());
            EXPECT_EQ(engine.cancelarProceso(1).codigo, CodigoResultado::OK);
            reinicio = std::thread([&engine, &reiniciado]() {
                engine.reiniciar();
                reiniciado = true;
            });
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            EXPECT_FALSE(reiniciado.load());  // espera al cierre de la rafaga
            paso.terminar();
        }
        reinicio.join();
        EXPECT_TRUE(reiniciado.load());

        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.procesos.size(), 0U);
        EXPECT_EQ(s.timeline.size(), 0U);
        EXPECT_EQ(s.eventos.size(), 0U);
        EXPECT_EQ(engine.cantidadWorkers(), 0U);

        engine.activarPuertaRafagaPruebas(false);
        EXPECT_TRUE(engine.agregarProceso(alta(1, 96, 100)));
        prepararPaso(engine);
        EXPECT_EQ(engine.siguientePaso().tiempoAvanzadoMs, 30);
        s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->estado, Estado::READY);  // no se cancelo
        EXPECT_EQ(contarEventos(s, TipoEvento::PROCESO_CANCELADO), 0);
    }

    // Caso 6: cancelar el ultimo proceso finaliza el sistema.
    {
        ConcurrentEngineV2 engine(Configuracion{1000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 50, 100)));
        EXPECT_EQ(engine.cancelarProceso(1).codigo, CodigoResultado::OK);
        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::FINALIZADO);
        EXPECT_EQ(contarEventos(s, TipoEvento::SISTEMA_FINALIZADO), 1);
        EXPECT_EQ(s.memoriaUsadaBytes, 0);
        EXPECT_EQ(engine.cantidadWorkers(), 0U);
    }

    return test::finish();
}
