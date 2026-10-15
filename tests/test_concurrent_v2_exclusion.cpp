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

    // Caso 4: el coordinador rechaza un worker cuyo PID/duracion son plausibles
    // pero cuyo motivo contradice el estado terminal esperado.
    {
        ConcurrentEngineV2 engine(Configuracion{256});
        engine.activarPuertaRafagaPruebas(true);
        EXPECT_TRUE(engine.agregarProceso(alta(70, 20, 5)));
        const EstadoSistema antes = engine.obtenerEstado();
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());

        PasoRetenido paso(engine, 70);
        EXPECT_TRUE(paso.enRun());
        engine.forzarMotivoWorkerPruebas(MotivoFinTurno::QUANTUM_AGOTADO);
        const ResultadoPaso resultado = paso.terminar();
        EXPECT_EQ(resultado.codigo, CodigoResultado::ESTADO_INVALIDO);

        const EstadoSistema despues = engine.obtenerEstado();
        const Proceso* proceso = buscar(despues, 70);
        EXPECT_TRUE(proceso != nullptr);
        if (proceso != nullptr) {
            EXPECT_EQ(proceso->estado, Estado::READY);
            EXPECT_EQ(proceso->tiempoCpuConsumidoMs, 0);
            EXPECT_EQ(proceso->tiempoCpuRestanteMs, 5);
            EXPECT_EQ(proceso->ticketReady, buscar(antes, 70)->ticketReady);
        }
        EXPECT_EQ(despues.tiempoSimuladoMs, 0);
        EXPECT_EQ(despues.memoriaUsadaBytes, 20);
        EXPECT_EQ(despues.timeline.size(), 0U);
        EXPECT_EQ(contarEventos(despues, TipoEvento::READY_A_RUN, 70), 0);
        EXPECT_EQ(engine.cantidadWorkers(), 1U);
        EXPECT_EQ(invariantesVioladas(despues), 0);

        PasoRetenido cancelado(engine, 70);
        EXPECT_TRUE(cancelado.enRun());
        engine.forzarMotivoWorkerPruebas(MotivoFinTurno::CANCELADO);
        EXPECT_EQ(cancelado.terminar().codigo,
                  CodigoResultado::ESTADO_INVALIDO);
        const EstadoSistema trasCancelado = engine.obtenerEstado();
        const Proceso* procesoTrasCancelado = buscar(trasCancelado, 70);
        EXPECT_TRUE(procesoTrasCancelado != nullptr);
        if (procesoTrasCancelado != nullptr) {
            EXPECT_EQ(procesoTrasCancelado->estado, Estado::READY);
        }
        EXPECT_EQ(trasCancelado.tiempoSimuladoMs, 0);
        EXPECT_EQ(trasCancelado.timeline.size(), 0U);
        EXPECT_EQ(contarEventos(trasCancelado, TipoEvento::READY_A_RUN, 70), 0);
    }

    // Caso 5: una excepción tras contabilizar un segmento revierte reloj, CPU,
    // eventos, timeline, memoria y ticket. El coordinador sigue operativo.
    {
        ConcurrentEngineV2 engine(Configuracion{256});
        EXPECT_TRUE(engine.agregarProceso(alta(80, 20, 40)));
        const EstadoSistema antes = engine.obtenerEstado();
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());
        engine.configurarFalloPasoPruebas(EtapaPasoPrueba::TRAS_SEGMENTO);

        const ResultadoPaso rechazado = engine.siguientePaso();
        EXPECT_EQ(rechazado.codigo, CodigoResultado::ESTADO_INVALIDO);
        const EstadoSistema despues = engine.obtenerEstado();
        const Proceso* proceso = buscar(despues, 80);
        EXPECT_TRUE(proceso != nullptr);
        if (proceso != nullptr) {
            EXPECT_EQ(proceso->estado, Estado::READY);
            EXPECT_EQ(proceso->tiempoCpuConsumidoMs, 0);
            EXPECT_EQ(proceso->tiempoCpuRestanteMs, 40);
            EXPECT_EQ(proceso->ticketReady, buscar(antes, 80)->ticketReady);
        }
        EXPECT_EQ(despues.tiempoSimuladoMs, 0);
        EXPECT_EQ(despues.memoriaUsadaBytes, 20);
        EXPECT_EQ(despues.timeline.size(), 0U);
        EXPECT_EQ(contarEventos(despues, TipoEvento::READY_A_RUN, 80), 0);
        EXPECT_EQ(contarEventos(despues, TipoEvento::CPU_EJECUTADA, 80), 0);
        EXPECT_EQ(invariantesVioladas(despues), 0);

        const ResultadoPaso siguiente = engine.siguientePaso();
        EXPECT_EQ(siguiente.codigo, CodigoResultado::OK);
        EXPECT_EQ(siguiente.tiempoAvanzadoMs, 30);
        const EstadoSistema trasRecuperacion = engine.obtenerEstado();
        const Proceso* procesoTrasRecuperacion = buscar(trasRecuperacion, 80);
        EXPECT_TRUE(procesoTrasRecuperacion != nullptr);
        if (procesoTrasRecuperacion != nullptr) {
            EXPECT_EQ(procesoTrasRecuperacion->tiempoCpuConsumidoMs, 30);
        }
    }

    // H-01 A: una cancelación RUN aceptada sobrevive al rechazo del resultado
    // del worker y se resuelve sobre READY sin confirmar CPU del turno fallido.
    {
        ConcurrentEngineV2 engine(Configuracion{512});
        engine.activarPuertaRafagaPruebas(true);
        EXPECT_TRUE(engine.agregarProceso(alta(90, 20, 100)));
        EXPECT_TRUE(engine.agregarProceso(alta(91, 20, 5)));
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());

        PasoRetenido paso(engine, 90);
        EXPECT_TRUE(paso.enRun());
        EXPECT_EQ(engine.cancelarProceso(90).codigo, CodigoResultado::OK);
        EXPECT_EQ(engine.cancelarProceso(90).codigo, CodigoResultado::OK);
        engine.forzarMotivoWorkerPruebas(MotivoFinTurno::PROCESO_TERMINADO);
        EXPECT_EQ(paso.terminar().codigo, CodigoResultado::ESTADO_INVALIDO);

        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 90)->estado, Estado::CANCEL);
        EXPECT_EQ(buscar(s, 90)->tiempoCpuConsumidoMs, 0);
        EXPECT_EQ(buscar(s, 90)->tiempoCpuRestanteMs, 100);
        EXPECT_EQ(s.memoriaUsadaBytes, 20);
        EXPECT_EQ(s.timeline.size(), 0U);
        EXPECT_EQ(contarEventos(s, TipoEvento::PROCESO_CANCELADO, 90), 1);
        EXPECT_EQ(contarEventos(s, TipoEvento::READY_A_RUN, 90), 0);
        EXPECT_EQ(engine.cantidadWorkers(), 1U);
        EXPECT_EQ(invariantesVioladas(s), 0);

        engine.activarPuertaRafagaPruebas(false);
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
        s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 91)->estado, Estado::FINISH);
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::FINALIZADO);
        EXPECT_EQ(s.memoriaUsadaBytes, 0);
        EXPECT_EQ(engine.cantidadWorkers(), 0U);
        engine.activarPuertaRafagaPruebas(false);
    }

    // H-01 B: al fallar la aplicación de CPU se revierte el turno y después se
    // aplica una sola vez la cancelación aceptada. El coordinador se recupera.
    {
        ConcurrentEngineV2 engine(Configuracion{256});
        engine.activarPuertaRafagaPruebas(true);
        EXPECT_TRUE(engine.agregarProceso(alta(92, 20, 80)));
        EXPECT_TRUE(engine.agregarProceso(alta(93, 20, 35)));
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());

        PasoRetenido paso(engine, 92);
        EXPECT_TRUE(paso.enRun());
        EXPECT_EQ(engine.cancelarProceso(92).codigo, CodigoResultado::OK);
        engine.configurarFalloPasoPruebas(EtapaPasoPrueba::TRAS_SEGMENTO);
        EXPECT_EQ(paso.terminar().codigo, CodigoResultado::ESTADO_INVALIDO);

        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 92)->estado, Estado::CANCEL);
        EXPECT_EQ(buscar(s, 92)->tiempoCpuConsumidoMs, 0);
        EXPECT_EQ(buscar(s, 92)->tiempoCpuRestanteMs, 80);
        EXPECT_EQ(s.tiempoSimuladoMs, 0);
        EXPECT_EQ(s.timeline.size(), 0U);
        EXPECT_EQ(contarEventos(s, TipoEvento::CPU_EJECUTADA, 92), 0);
        EXPECT_EQ(contarEventos(s, TipoEvento::PROCESO_CANCELADO, 92), 1);
        EXPECT_EQ(s.memoriaUsadaBytes, 20);
        EXPECT_EQ(engine.cantidadWorkers(), 1U);
        EXPECT_TRUE(s.estadoEjecucion != EstadoEjecucion::FINALIZADO);
        EXPECT_EQ(engine.cancelarProceso(92).codigo,
                  CodigoResultado::PID_YA_CANCELADO);
        EXPECT_EQ(contarEventos(engine.obtenerEstado(),
                                TipoEvento::PROCESO_CANCELADO, 92), 1);
        engine.activarPuertaRafagaPruebas(false);

        // Un segundo fallo consecutivo de otro turno no mata al coordinador;
        // el turno siguiente se procesa normalmente hasta finalizar el sistema.
        engine.configurarFalloPasoPruebas(EtapaPasoPrueba::TRAS_SEGMENTO);
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::ESTADO_INVALIDO);
        EXPECT_EQ(buscar(engine.obtenerEstado(), 93)->estado, Estado::READY);
        EXPECT_EQ(engine.siguientePaso().tiempoAvanzadoMs, 30);
        EXPECT_EQ(engine.siguientePaso().tiempoAvanzadoMs, 5);
        s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 93)->estado, Estado::FINISH);
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::FINALIZADO);
        EXPECT_EQ(s.memoriaUsadaBytes, 0);
        EXPECT_EQ(engine.cantidadWorkers(), 0U);
        engine.activarPuertaRafagaPruebas(false);
    }

    // H-02: una excepción en cualquier fase del salto idle/despertar revierte
    // reloj, tickets, procesos, espera y eventos como una sola transacción.
    {
        const EtapaPasoPrueba fallos[] = {
            EtapaPasoPrueba::ANTES_CONFIRMAR_IDLE,
            EtapaPasoPrueba::TRAS_EVENTOS_IDLE,
            EtapaPasoPrueba::TRAS_TICKETS_DESPERTAR_IDLE,
            EtapaPasoPrueba::TRAS_ESTADO_DESPERTAR_IDLE};
        for (const EtapaPasoPrueba fallo : fallos) {
            ConcurrentEngineV2 engine(Configuracion{512});
            EXPECT_TRUE(engine.agregarProceso(
                alta(100, 20, 100, OperacionIO{true, 5, 15})));
            EXPECT_TRUE(engine.iniciar());
            EXPECT_TRUE(engine.pausar());
            EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
            EXPECT_EQ(buscar(engine.obtenerEstado(), 100)->estado, Estado::WAIT);

            const EstadoSistema antes = engine.obtenerEstado();
            engine.configurarFalloPasoPruebas(fallo);
            const ResultadoPaso rechazado = engine.siguientePaso();
            EXPECT_EQ(rechazado.codigo, CodigoResultado::ESTADO_INVALIDO);
            const EstadoSistema despues = engine.obtenerEstado();
            EXPECT_EQ(diferencia(antes, despues), std::string{});
            EXPECT_EQ(invariantesVioladas(despues), 0);

            EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
            const EstadoSistema recuperado = engine.obtenerEstado();
            EXPECT_EQ(recuperado.tiempoSimuladoMs, 20);
            EXPECT_EQ(buscar(recuperado, 100)->estado, Estado::READY);
            EXPECT_EQ(contarEventos(recuperado, TipoEvento::WAIT_A_READY, 100), 1);
        }
    }

    // H-02: dos procesos con el mismo vencimiento despiertan juntos y reciben
    // tickets nuevos; agotar tickets en ese límite no publica cambio parcial.
    {
        ConcurrentEngineV2 engine(Configuracion{512});
        EXPECT_TRUE(engine.agregarProceso(
            alta(110, 20, 100, OperacionIO{true, 5, 5})));
        EXPECT_TRUE(engine.agregarProceso(
            alta(111, 30, 100, OperacionIO{true, 10, 10})));
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 110)->estado, Estado::WAIT);
        EXPECT_EQ(buscar(s, 111)->estado, Estado::WAIT);
        EXPECT_EQ(s.tiempoSimuladoMs, 15);
        const EstadoSistema antes = s;

        engine.fijarSiguienteTicketPruebas(UINT64_MAX);
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::ESTADO_INVALIDO);
        s = engine.obtenerEstado();
        EXPECT_EQ(diferencia(antes, s), std::string{});

        engine.fijarSiguienteTicketPruebas(UINT64_MAX - 2);
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
        s = engine.obtenerEstado();
        EXPECT_EQ(s.tiempoSimuladoMs, 20);
        EXPECT_EQ(buscar(s, 110)->estado, Estado::READY);
        EXPECT_EQ(buscar(s, 111)->estado, Estado::READY);
        EXPECT_TRUE(buscar(s, 111)->ticketReady < buscar(s, 110)->ticketReady);
        EXPECT_EQ(contarEventos(s, TipoEvento::WAIT_A_READY), 2);
        EXPECT_EQ(invariantesVioladas(s), 0);
    }

    return test::finish();
}
