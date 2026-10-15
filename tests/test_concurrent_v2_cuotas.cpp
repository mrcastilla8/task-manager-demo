// Cuotas por prioridad (DP-04) y recalculo dinamico P1/P2/P3 (DP-02) del motor
// concurrente, usando la lista de Castilla y el Scheduler de Brickzon.
#include "tests/ConcurrentV2Support.h"
#include "tests/TestSupport.h"

#include <vector>

using namespace ConcurrentSimulator::v2;
using namespace ConcurrentSimulator::v2::testing;

namespace {

std::vector<std::int64_t> duraciones(const EstadoSistema& s, int pid) {
    std::vector<std::int64_t> resultado;
    for (const BloqueTimeline& bloque : s.timeline) {
        if (bloque.pid() == pid) {
            resultado.push_back(bloque.finMs() - bloque.inicioMs());
        }
    }
    return resultado;
}

void prepararPaso(ConcurrentEngineV2& engine) {
    EXPECT_TRUE(engine.iniciar());
    EXPECT_TRUE(engine.pausar());
}

}  // namespace

int main() {
    // Caso 1: 10/20/30 ms observados en un solo escenario. X (grande) espera en
    // WAIT sin salir de la poblacion TL, asi que Y (mediano) es P2 y Z (pequeno)
    // es P1; cuando solo queda X vuelve a ser P3.
    {
        ConcurrentEngineV2 engine(Configuracion{100000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 96, 50, {true, 5, 1000})));  // X
        EXPECT_TRUE(engine.agregarProceso(alta(2, 40, 100)));                 // Y
        EXPECT_TRUE(engine.agregarProceso(alta(3, 12, 100)));                 // Z
        prepararPaso(engine);
        while (engine.obtenerEstado().estadoEjecucion !=
               EstadoEjecucion::FINALIZADO) {
            EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
            EXPECT_TRUE(prioridadesCoherentes(engine.obtenerEstado()));
        }
        const EstadoSistema s = engine.obtenerEstado();
        const std::vector<std::int64_t> y(5, 20);   // P2: 5 x 20 ms
        const std::vector<std::int64_t> z(10, 10);  // P1: 10 x 10 ms
        EXPECT_TRUE(duraciones(s, 2) == y);
        EXPECT_TRUE(duraciones(s, 3) == z);
        // X: 5 ms hasta E/S y, al volver como unico proceso (P3), 30 + 15 ms.
        const std::vector<std::int64_t> x = {5, 30, 15};
        EXPECT_TRUE(duraciones(s, 1) == x);

        // Orden y huecos: X [0,5), Y 5x20, Z 10x10, idle hasta 1005 y X al final.
        EXPECT_EQ(s.timeline.front().pid(), 1);
        EXPECT_EQ(s.timeline[1].inicioMs(), 5);
        EXPECT_EQ(s.timeline[6].pid(), 3);
        EXPECT_EQ(s.timeline[6].inicioMs(), 105);
        EXPECT_EQ(s.timeline[16].pid(), 1);
        EXPECT_EQ(s.timeline[16].inicioMs(), 1005);
        EXPECT_EQ(invariantesVioladas(s), 0);
    }

    // Caso 2: la cuota se fija al despachar. Una alta durante RUN reclasifica a
    // los demas pero no cambia la cuota del turno en curso (DP-02c / DP-04).
    {
        ConcurrentEngineV2 engine(Configuracion{100000});
        engine.activarPuertaRafagaPruebas(true);
        EXPECT_TRUE(engine.agregarProceso(alta(1, 96, 100)));
        EXPECT_TRUE(engine.agregarProceso(alta(2, 40, 100)));
        EXPECT_TRUE(engine.agregarProceso(alta(3, 12, 100)));
        prepararPaso(engine);

        {
            PasoRetenido paso(engine, 1);
            EXPECT_TRUE(paso.enRun());
            EstadoSistema s = engine.obtenerEstado();
            EXPECT_EQ(s.pidEjecutando, 1);
            EXPECT_EQ(s.cuotaTurnoFijadaMs, 30);
            EXPECT_EQ(buscar(s, 1)->prioridad, Prioridad::P3);
            EXPECT_EQ(s.cpuConsumidaTurnoMs, 0);

            // Un proceso enorme desplaza las bandas: el que corre pasa a P1.
            EXPECT_TRUE(engine.agregarProceso(alta(4, 1000, 100)));
            s = engine.obtenerEstado();
            EXPECT_EQ(buscar(s, 1)->prioridad, Prioridad::P1);
            EXPECT_EQ(buscar(s, 4)->prioridad, Prioridad::P3);
            EXPECT_EQ(s.cuotaTurnoFijadaMs, 30);  // la cuota del turno no cambia
            EXPECT_EQ(s.pidEjecutando, 1);
            EXPECT_TRUE(prioridadesCoherentes(s));
            EXPECT_EQ(invariantesVioladas(s), 0);

            const ResultadoPaso r = paso.terminar();
            EXPECT_EQ(r.tiempoAvanzadoMs, 30);
        }
        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.timeline.size(), 1U);
        EXPECT_EQ(s.timeline[0].finMs() - s.timeline[0].inicioMs(), 30);

        // El siguiente despacho usa la clase vigente: el proceso 4 es P3 (30 ms).
        {
            PasoRetenido paso(engine, 4);
            EXPECT_TRUE(paso.enRun());
            EXPECT_EQ(engine.obtenerEstado().cuotaTurnoFijadaMs, 30);
            EXPECT_EQ(paso.terminar().tiempoAvanzadoMs, 30);
        }
        engine.activarPuertaRafagaPruebas(false);
    }

    // Caso 3: recalculo tras alta admitida (DP-02b), con evento por cada cambio.
    {
        ConcurrentEngineV2 engine(Configuracion{100000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 96, 100)));
        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->prioridad, Prioridad::P3);  // A == B -> P3
        EXPECT_EQ(contarEventos(s, TipoEvento::PRIORIDAD_CAMBIADA), 0);

        EXPECT_TRUE(engine.agregarProceso(alta(2, 12, 100)));
        s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->prioridad, Prioridad::P3);
        EXPECT_EQ(buscar(s, 2)->prioridad, Prioridad::P1);
        EXPECT_EQ(contarEventos(s, TipoEvento::PRIORIDAD_CAMBIADA, 2), 1);

        EXPECT_TRUE(engine.agregarProceso(alta(3, 40, 100)));
        s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 3)->prioridad, Prioridad::P2);
        EXPECT_EQ(buscar(s, 1)->prioridad, Prioridad::P3);
        EXPECT_EQ(buscar(s, 2)->prioridad, Prioridad::P1);
        EXPECT_TRUE(prioridadesCoherentes(s));
    }

    // Caso 4: recalculo tras FINISH antes del siguiente despacho, y FINISH deja
    // de contar en la poblacion TL.
    {
        ConcurrentEngineV2 engine(Configuracion{100000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 96, 20)));   // termina en 1 paso
        EXPECT_TRUE(engine.agregarProceso(alta(2, 40, 100)));
        EXPECT_TRUE(engine.agregarProceso(alta(3, 12, 100)));
        prepararPaso(engine);

        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 2)->prioridad, Prioridad::P2);

        EXPECT_EQ(engine.siguientePaso().tiempoAvanzadoMs, 20);  // A termina
        s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->estado, Estado::FINISH);
        // TL = {40, 12}: el mediano pasa a P3 y el pequeno sigue en P1.
        EXPECT_EQ(buscar(s, 2)->prioridad, Prioridad::P3);
        EXPECT_EQ(buscar(s, 3)->prioridad, Prioridad::P1);
        EXPECT_TRUE(prioridadesCoherentes(s));

        // El evento de prioridad es posterior al fin del proceso.
        std::size_t indiceFin = 0;
        std::size_t indicePrioridad = 0;
        for (std::size_t i = 0; i < s.eventos.size(); ++i) {
            if (s.eventos[i].tipo == TipoEvento::RUN_A_FINISH) indiceFin = i;
            if (s.eventos[i].tipo == TipoEvento::PRIORIDAD_CAMBIADA &&
                s.eventos[i].pid == 2) {
                indicePrioridad = i;
            }
        }
        EXPECT_TRUE(indicePrioridad > indiceFin);

        // El despacho siguiente ya usa la clase nueva: 30 ms para el proceso 2.
        EXPECT_EQ(engine.siguientePaso().tiempoAvanzadoMs, 30);
        EXPECT_EQ(engine.obtenerEstado().timeline.back().pid(), 2);
    }

    // Caso 5: recalculo tras CANCEL.
    {
        ConcurrentEngineV2 engine(Configuracion{100000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 96, 100)));
        EXPECT_TRUE(engine.agregarProceso(alta(2, 40, 100)));
        EXPECT_TRUE(engine.agregarProceso(alta(3, 12, 100)));
        EXPECT_TRUE(engine.cancelarProceso(1));
        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 2)->prioridad, Prioridad::P3);
        EXPECT_EQ(buscar(s, 3)->prioridad, Prioridad::P1);
        EXPECT_TRUE(prioridadesCoherentes(s));
        // El orden de los eventos: cancelacion y despues el cambio de prioridad.
        std::size_t indiceCancel = 0;
        std::size_t indicePrioridad = 0;
        for (std::size_t i = 0; i < s.eventos.size(); ++i) {
            if (s.eventos[i].tipo == TipoEvento::PROCESO_CANCELADO) indiceCancel = i;
            if (s.eventos[i].tipo == TipoEvento::PRIORIDAD_CAMBIADA &&
                s.eventos[i].pid == 2) {
                indicePrioridad = i;
            }
        }
        EXPECT_TRUE(indicePrioridad > indiceCancel);
    }

    // Caso 6: una alta rechazada no altera la poblacion TL ni las prioridades.
    {
        ConcurrentEngineV2 engine(Configuracion{200});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 96, 100)));
        EXPECT_TRUE(engine.agregarProceso(alta(2, 40, 100)));
        const EstadoSistema antes = engine.obtenerEstado();
        EXPECT_EQ(engine.agregarProceso(alta(3, 5000, 100)).codigo,
                  CodigoResultado::MEMORIA_INSUFICIENTE);
        EXPECT_EQ(engine.agregarProceso(alta(2, 10, 100)).codigo,
                  CodigoResultado::PID_DUPLICADO);
        EXPECT_EQ(engine.agregarProceso(alta(4, 10, 0)).codigo,
                  CodigoResultado::ARGUMENTO_INVALIDO);
        EXPECT_TRUE(diferencia(antes, engine.obtenerEstado()).empty());
    }

    // Caso 7: sin aging. La espera es solo una metrica: un proceso de baja
    // prioridad espera mucho y su clase no cambia (DP-10).
    {
        ConcurrentEngineV2 engine(Configuracion{100000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 96, 3000)));  // P3, acapara
        EXPECT_TRUE(engine.agregarProceso(alta(2, 12, 60)));    // P1, espera
        prepararPaso(engine);
        for (int i = 0; i < 90; ++i) {
            EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
        }
        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.tiempoSimuladoMs, 2700);
        EXPECT_EQ(buscar(s, 2)->prioridad, Prioridad::P1);
        EXPECT_EQ(buscar(s, 2)->tiempoEsperaMs, 2700);  // metrica acumulada
        EXPECT_EQ(buscar(s, 2)->tiempoCpuConsumidoMs, 0);
        for (const BloqueTimeline& bloque : s.timeline) {
            EXPECT_EQ(bloque.pid(), 1);
        }
        EXPECT_EQ(contarEventos(s, TipoEvento::PRIORIDAD_CAMBIADA, 2), 1);  // el del alta
    }

    return test::finish();
}
