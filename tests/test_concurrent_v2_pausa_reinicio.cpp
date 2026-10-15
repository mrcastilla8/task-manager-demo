// Iniciar, pausar y reiniciar del motor concurrente v2. Hasta que G1 ratifique
// la ejecucion continua, iniciar() solo fija el estado EJECUTANDO (igual que el
// motor logico) y el avance es siguientePaso() con el motor PAUSADO.
#include "tests/ConcurrentV2Support.h"
#include "tests/TestSupport.h"

#include <atomic>
#include <chrono>
#include <thread>

using namespace ConcurrentSimulator::v2;
using namespace ConcurrentSimulator::v2::testing;

int main() {
    // Caso 1: codigos de retorno segun el estado.
    {
        ConcurrentEngineV2 engine(Configuracion{1000});
        EXPECT_EQ(engine.pausar().codigo, CodigoResultado::ESTADO_INVALIDO);
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::ESTADO_INVALIDO);
        EXPECT_EQ(engine.iniciar().codigo, CodigoResultado::OK);
        EXPECT_EQ(engine.iniciar().codigo, CodigoResultado::OK);  // idempotente
        EXPECT_EQ(engine.obtenerEstado().estadoEjecucion, EstadoEjecucion::EJECUTANDO);
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::ESTADO_INVALIDO);
        EXPECT_EQ(engine.pausar().codigo, CodigoResultado::OK);
        EXPECT_EQ(engine.pausar().codigo, CodigoResultado::ESTADO_INVALIDO);
        EXPECT_EQ(engine.obtenerEstado().estadoEjecucion, EstadoEjecucion::PAUSADO);
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);  // sin trabajo
    }

    // Caso 2: iniciar() no despacha ni avanza el reloj.
    {
        ConcurrentEngineV2 engine(Configuracion{1000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 50, 100)));
        EXPECT_TRUE(engine.iniciar());
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::EJECUTANDO);
        EXPECT_EQ(s.tiempoSimuladoMs, 0);
        EXPECT_EQ(s.timeline.size(), 0U);
        EXPECT_EQ(s.pidEjecutando, -1);
        EXPECT_EQ(contarEventos(s, TipoEvento::READY_A_RUN), 0);
        EXPECT_TRUE(engine.pausar());
        EXPECT_EQ(engine.siguientePaso().tiempoAvanzadoMs, 30);
    }

    // Caso 3: pausar() no retorna hasta cerrar la rafaga en curso, y un
    // iniciar() durante la rafaga no abre una segunda unidad logica.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        engine.activarPuertaRafagaPruebas(true);
        EXPECT_TRUE(engine.agregarProceso(alta(1, 50, 200)));
        EXPECT_TRUE(engine.agregarProceso(alta(2, 50, 200)));
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());

        std::atomic<bool> pausado{false};
        Resultado resultadoPausa;
        std::thread hiloPausa;
        {
            PasoRetenido paso(engine);
            EXPECT_TRUE(paso.enRun());
            EXPECT_EQ(engine.iniciar().codigo, CodigoResultado::OK);  // mientras corre
            hiloPausa = std::thread([&]() {
                resultadoPausa = engine.pausar();
                pausado = true;
            });
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            EXPECT_FALSE(pausado.load());  // espera al cierre de la rafaga
            EXPECT_EQ(invariantesVioladas(engine.obtenerEstado()), 0);
            paso.terminar();
        }
        hiloPausa.join();
        EXPECT_TRUE(pausado.load());
        EXPECT_EQ(resultadoPausa.codigo, CodigoResultado::OK);
        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::PAUSADO);
        EXPECT_EQ(s.pidEjecutando, -1);
        EXPECT_EQ(s.timeline.size(), 1U);  // una sola unidad, no una segunda
        engine.activarPuertaRafagaPruebas(false);
    }

    // Caso 4: reiniciar() limpia la sesion y conserva la configuracion.
    {
        ConcurrentEngineV2 engine(Configuracion{1234});
        for (int pid = 1; pid <= 3; ++pid) {
            EXPECT_TRUE(engine.agregarProceso(alta(pid, 100 * pid, 200)));
        }
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());
        for (int i = 0; i < 4; ++i) engine.siguientePaso();
        EXPECT_TRUE(engine.reiniciar());

        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::DETENIDO);
        EXPECT_EQ(s.tiempoSimuladoMs, 0);
        EXPECT_EQ(s.procesos.size(), 0U);
        EXPECT_EQ(s.eventos.size(), 0U);
        EXPECT_EQ(s.timeline.size(), 0U);
        EXPECT_EQ(s.pidEjecutando, -1);
        EXPECT_EQ(s.memoriaUsadaBytes, 0);
        EXPECT_EQ(s.memoriaTotalBytes, 1234);  // capacidad conservada
        EXPECT_EQ(engine.cantidadWorkers(), 0U);

        // Se reutiliza: los tickets vuelven a empezar en 1.
        EXPECT_TRUE(engine.agregarProceso(alta(1, 100, 50)));
        EXPECT_EQ(buscar(engine.obtenerEstado(), 1)->ticketReady, 1U);
    }

    // Caso 5: reiniciar() durante una rafaga espera a que cierre.
    {
        ConcurrentEngineV2 engine(Configuracion{1000});
        engine.activarPuertaRafagaPruebas(true);
        EXPECT_TRUE(engine.agregarProceso(alta(1, 50, 100)));
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());

        std::atomic<bool> reiniciado{false};
        std::thread reinicio;
        {
            PasoRetenido paso(engine, 1);
            EXPECT_TRUE(paso.enRun());
            reinicio = std::thread([&]() {
                engine.reiniciar();
                reiniciado = true;
            });
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            EXPECT_FALSE(reiniciado.load());
            paso.terminar();
        }
        reinicio.join();
        EXPECT_TRUE(reiniciado.load());
        EXPECT_EQ(engine.obtenerEstado().procesos.size(), 0U);
        EXPECT_EQ(engine.cantidadWorkers(), 0U);
    }

    // Caso 6: dos reinicios simultaneos no se bloquean.
    {
        ConcurrentEngineV2 engine(Configuracion{1000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 50, 100)));
        Resultado a;
        Resultado b;
        std::thread t1([&]() { a = engine.reiniciar(); });
        std::thread t2([&]() { b = engine.reiniciar(); });
        t1.join();
        t2.join();
        EXPECT_EQ(a.codigo, CodigoResultado::OK);
        EXPECT_EQ(b.codigo, CodigoResultado::OK);
        EXPECT_EQ(engine.cantidadWorkers(), 0U);
    }

    // Caso 7: FINALIZADO no se puede iniciar; una alta reabre la sesion.
    {
        ConcurrentEngineV2 engine(Configuracion{1000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 50, 20)));
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
        EXPECT_EQ(engine.obtenerEstado().estadoEjecucion, EstadoEjecucion::FINALIZADO);
        EXPECT_EQ(engine.iniciar().codigo, CodigoResultado::ESTADO_INVALIDO);

        EXPECT_TRUE(engine.agregarProceso(alta(2, 50, 20)));
        EXPECT_EQ(engine.obtenerEstado().estadoEjecucion, EstadoEjecucion::DETENIDO);
        EXPECT_TRUE(engine.iniciar());
    }

    return test::finish();
}
