#include "tests/ConcurrentV2Support.h"
#include "tests/TestSupport.h"

#include <vector>

using namespace ConcurrentSimulator::v2;
using namespace ConcurrentSimulator::v2::testing;

int main() {
    // Caso 1: estado inicial y validaciones de admision.
    {
        ConcurrentEngineV2 engine(Configuracion{1000});
        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::DETENIDO);
        EXPECT_EQ(s.memoriaTotalBytes, 1000);
        EXPECT_EQ(s.memoriaUsadaBytes, 0);
        EXPECT_EQ(s.procesos.size(), 0U);
        EXPECT_EQ(s.pidEjecutando, -1);
        EXPECT_EQ(engine.cantidadWorkers(), 0U);

        EXPECT_EQ(engine.agregarProceso(alta(0, 10, 50)).codigo,
                  CodigoResultado::ARGUMENTO_INVALIDO);
        EXPECT_EQ(engine.agregarProceso(alta(1, 400, 50)).codigo,
                  CodigoResultado::OK);
        EXPECT_EQ(engine.agregarProceso(alta(1, 10, 50)).codigo,
                  CodigoResultado::PID_DUPLICADO);
        EXPECT_EQ(engine.agregarProceso(alta(2, 700, 50)).codigo,
                  CodigoResultado::MEMORIA_INSUFICIENTE);
        EXPECT_EQ(engine.agregarProceso(alta(3, 10, 0)).codigo,
                  CodigoResultado::ARGUMENTO_INVALIDO);

        s = engine.obtenerEstado();
        EXPECT_EQ(s.procesos.size(), 1U);
        EXPECT_EQ(s.memoriaUsadaBytes, 400);
        EXPECT_EQ(engine.cantidadWorkers(), 1U);
        EXPECT_EQ(buscar(s, 1)->ticketReady, 1U);
        EXPECT_EQ(invariantesVioladas(s), 0);
    }

    // Caso 2: ciclo con E/S: secuencia exacta de eventos y bloques de timeline.
    {
        ConcurrentEngineV2 engine(Configuracion{1000});
        // P3 (proceso unico): cuota 30. E/S tras 8 ms de CPU, espera de 10 ms.
        EXPECT_TRUE(engine.agregarProceso(alta(1, 50, 40, {true, 8, 10})));
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());

        // Paso 1: 8 ms de CPU y pasa a WAIT.
        ResultadoPaso r = engine.siguientePaso();
        EXPECT_EQ(r.codigo, CodigoResultado::OK);
        EXPECT_TRUE(r.avanzo);
        EXPECT_TRUE(r.ejecutoCpu);
        EXPECT_EQ(r.tiempoAvanzadoMs, 8);
        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->estado, Estado::WAIT);
        EXPECT_EQ(s.memoriaUsadaBytes, 50);  // WAIT conserva la memoria
        EXPECT_EQ(s.pidEjecutando, -1);
        EXPECT_EQ(s.colaReady.size(), 0U);

        // Paso 2: sin READY, el reloj salta al despertar (t = 18): sin bloque.
        r = engine.siguientePaso();
        EXPECT_EQ(r.codigo, CodigoResultado::OK);
        EXPECT_TRUE(r.avanzo);
        EXPECT_FALSE(r.ejecutoCpu);
        EXPECT_EQ(r.tiempoAvanzadoMs, 10);
        s = engine.obtenerEstado();
        EXPECT_EQ(s.tiempoSimuladoMs, 18);
        EXPECT_EQ(s.timeline.size(), 1U);
        EXPECT_EQ(buscar(s, 1)->estado, Estado::READY);

        // Pasos 3 y 4: 30 ms (cuota) y 2 ms restantes hasta terminar.
        EXPECT_EQ(engine.siguientePaso().tiempoAvanzadoMs, 30);
        EXPECT_EQ(engine.siguientePaso().tiempoAvanzadoMs, 2);
        s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->estado, Estado::FINISH);
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::FINALIZADO);
        EXPECT_EQ(s.memoriaUsadaBytes, 0);
        EXPECT_EQ(engine.cantidadWorkers(), 0U);  // FINISH retira al worker

        EXPECT_EQ(s.timeline.size(), 3U);
        EXPECT_EQ(s.timeline[0].inicioMs(), 0);
        EXPECT_EQ(s.timeline[0].finMs(), 8);
        EXPECT_EQ(s.timeline[1].inicioMs(), 18);  // hueco idle [8, 18)
        EXPECT_EQ(s.timeline[1].finMs(), 48);
        EXPECT_EQ(s.timeline[2].inicioMs(), 48);
        EXPECT_EQ(s.timeline[2].finMs(), 50);
        EXPECT_EQ(invariantesVioladas(s), 0);

        const std::vector<TipoEvento> esperado = {
            TipoEvento::PROCESO_CREADO,  TipoEvento::PROCESO_ADMITIDO,
            TipoEvento::READY_A_RUN,
            TipoEvento::CPU_EJECUTADA,   TipoEvento::RUN_A_WAIT,
            TipoEvento::WAIT_A_READY,    TipoEvento::READY_A_RUN,
            TipoEvento::CPU_EJECUTADA,   TipoEvento::CUOTA_AGOTADA,
            TipoEvento::RUN_A_READY,     TipoEvento::READY_A_RUN,
            TipoEvento::CPU_EJECUTADA,   TipoEvento::RUN_A_FINISH};
        EXPECT_TRUE(tiposDe(s, 1) == esperado);
        EXPECT_EQ(contarEventos(s, TipoEvento::CPU_IDLE_INICIO), 1);
        EXPECT_EQ(contarEventos(s, TipoEvento::CPU_IDLE_FIN), 1);
        EXPECT_EQ(contarEventos(s, TipoEvento::SISTEMA_FINALIZADO), 1);
    }

    // Caso 3: la suma de las rafagas es el CPU total de cada proceso y no hay
    // tiempo ocioso sin E/S.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        std::int64_t total = 0;
        for (int pid = 1; pid <= 5; ++pid) {
            const std::int64_t cpu = 40 + 13 * pid;
            total += cpu;
            EXPECT_TRUE(engine.agregarProceso(alta(pid, 20 * pid, cpu)));
        }
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());
        int pasos = 0;
        while (engine.obtenerEstado().estadoEjecucion !=
                   EstadoEjecucion::FINALIZADO &&
               pasos < 1000) {
            EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
            EXPECT_EQ(invariantesVioladas(engine.obtenerEstado()), 0);
            EXPECT_TRUE(prioridadesCoherentes(engine.obtenerEstado()));
            ++pasos;
        }
        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::FINALIZADO);
        EXPECT_EQ(s.tiempoSimuladoMs, total);
        std::int64_t suma = 0;
        for (const BloqueTimeline& bloque : s.timeline) {
            suma += bloque.finMs() - bloque.inicioMs();
        }
        EXPECT_EQ(suma, total);
        for (const Proceso& proceso : s.procesos) {
            EXPECT_EQ(proceso.estado, Estado::FINISH);
            EXPECT_EQ(proceso.tiempoCpuConsumidoMs, proceso.tiempoCpuTotalMs);
        }
        EXPECT_EQ(s.memoriaUsadaBytes, 0);
        EXPECT_EQ(engine.cantidadWorkers(), 0U);
    }

    // Caso 4: sin procesos o sin trabajo el paso no avanza.
    {
        ConcurrentEngineV2 engine(Configuracion{100});
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());
        const ResultadoPaso r = engine.siguientePaso();
        EXPECT_EQ(r.codigo, CodigoResultado::OK);
        EXPECT_FALSE(r.avanzo);
        EXPECT_FALSE(r.ejecutoCpu);
        EXPECT_EQ(r.tiempoAvanzadoMs, 0);
    }

    // Caso 5: ciclo de vida de los hilos (construir/destruir sin fugas ni bloqueos).
    {
        for (int i = 0; i < 100; ++i) {
            ConcurrentEngineV2 vacio(Configuracion{100});
        }
        {
            ConcurrentEngineV2 engine(Configuracion{10000});
            for (int pid = 1; pid <= 8; ++pid) {
                EXPECT_TRUE(engine.agregarProceso(alta(pid, 10 + pid, 200)));
            }
            EXPECT_EQ(engine.cantidadWorkers(), 8U);
            // Se destruye con ocho workers activos.
        }
        {
            ConcurrentEngineV2 engine(Configuracion{10000});
            for (int pid = 1; pid <= 4; ++pid) {
                EXPECT_TRUE(engine.agregarProceso(alta(pid, 10 + pid, 200)));
            }
            EXPECT_TRUE(engine.iniciar());
            EXPECT_TRUE(engine.pausar());
            for (int i = 0; i < 5; ++i) engine.siguientePaso();
            EXPECT_TRUE(engine.cancelarProceso(2));
            EXPECT_EQ(engine.cantidadWorkers(), 3U);
            EXPECT_TRUE(engine.reiniciar());
            EXPECT_EQ(engine.cantidadWorkers(), 0U);
        }
    }

    return test::finish();
}
