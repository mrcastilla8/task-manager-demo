// Tickets READY (DP-05) con AsignadorTickets de Brickzon: Round Robin, retorno
// de WAIT y agotamiento del contador con reserva exacta de tickets.
#include "tests/ConcurrentV2Support.h"
#include "tests/TestSupport.h"

#include <cstdint>
#include <limits>
#include <set>
#include <vector>

using namespace ConcurrentSimulator::v2;
using namespace ConcurrentSimulator::v2::testing;

namespace {

constexpr std::uint64_t MAXIMO = (std::numeric_limits<std::uint64_t>::max)();

std::vector<int> pidsDelTimeline(const EstadoSistema& s) {
    std::vector<int> pids;
    for (const BloqueTimeline& bloque : s.timeline) {
        pids.push_back(bloque.pid());
    }
    return pids;
}

void prepararPaso(ConcurrentEngineV2& engine) {
    EXPECT_TRUE(engine.iniciar());
    EXPECT_TRUE(engine.pausar());
}

}  // namespace

int main() {
    // Caso 1: Round Robin. Dos procesos de la misma clase alternan por turnos y
    // los tickets READY son siempre distintos y crecientes.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 50, 100)));
        EXPECT_TRUE(engine.agregarProceso(alta(2, 50, 100)));
        prepararPaso(engine);

        std::uint64_t ultimo = 0;
        for (int paso = 0; paso < 8; ++paso) {
            EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
            const EstadoSistema s = engine.obtenerEstado();
            std::set<std::uint64_t> tickets;
            for (const Proceso& p : s.procesos) {
                if (p.estado == Estado::READY) {
                    EXPECT_TRUE(p.ticketReady > 0);
                    tickets.insert(p.ticketReady);
                    ultimo = (std::max)(ultimo, p.ticketReady);
                }
            }
            std::size_t listos = 0;
            for (const Proceso& p : s.procesos) {
                if (p.estado == Estado::READY) ++listos;
            }
            EXPECT_EQ(tickets.size(), listos);
        }
        const EstadoSistema s = engine.obtenerEstado();
        const std::vector<int> esperado = {1, 2, 1, 2, 1, 2, 1, 2};
        EXPECT_TRUE(pidsDelTimeline(s) == esperado);
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::FINALIZADO);
        EXPECT_TRUE(ultimo > 2);
    }

    // Caso 2: un proceso que vuelve de WAIT queda detras de sus pares segun el
    // instante en que despierta.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 50, 100, {true, 10, 30})));
        EXPECT_TRUE(engine.agregarProceso(alta(2, 50, 100)));
        EXPECT_TRUE(engine.agregarProceso(alta(3, 50, 100)));
        prepararPaso(engine);
        for (int i = 0; i < 6; ++i) {
            EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
        }
        // P1 corre 10 ms y pasa a WAIT; despierta en t=40, justo al terminar el
        // turno de P2, y reingresa antes que P2 (que agota su cuota despues).
        const std::vector<int> esperado = {1, 2, 3, 1, 2, 3};
        EXPECT_TRUE(pidsDelTimeline(engine.obtenerEstado()) == esperado);
    }

    // Caso 3: el alta falla sin efectos cuando no quedan tickets.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        engine.fijarSiguienteTicketPruebas(MAXIMO);
        EXPECT_EQ(engine.agregarProceso(alta(1, 500, 50)).codigo,
                  CodigoResultado::ESTADO_INVALIDO);
        EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.procesos.size(), 0U);
        EXPECT_EQ(s.eventos.size(), 0U);
        EXPECT_EQ(s.memoriaUsadaBytes, 0);
        EXPECT_EQ(engine.cantidadWorkers(), 0U);

        // Queda exactamente un ticket utilizable (MAXIMO - 1).
        engine.fijarSiguienteTicketPruebas(MAXIMO - 1);
        EXPECT_TRUE(engine.agregarProceso(alta(1, 500, 50)));
        EXPECT_EQ(buscar(engine.obtenerEstado(), 1)->ticketReady, MAXIMO - 1);
        const EstadoSistema antes = engine.obtenerEstado();
        EXPECT_EQ(engine.agregarProceso(alta(2, 500, 50)).codigo,
                  CodigoResultado::ESTADO_INVALIDO);
        EXPECT_TRUE(diferencia(antes, engine.obtenerEstado()).empty());
        EXPECT_EQ(engine.cantidadWorkers(), 1U);
    }

    // Caso 4 (finalizacion): un paso que termina el proceso no consume tickets, asi
    // que se puede ejecutar con el contador completamente agotado.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 50, 20)));  // termina en un paso
        engine.fijarSiguienteTicketPruebas(MAXIMO);           // 0 tickets libres
        prepararPaso(engine);
        const ResultadoPaso r = engine.siguientePaso();
        EXPECT_EQ(r.codigo, CodigoResultado::OK);
        EXPECT_EQ(r.tiempoAvanzadoMs, 20);
        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->estado, Estado::FINISH);
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::FINALIZADO);
    }

    // Caso 5 (fin de cuota): si el paso necesita un ticket y no hay, se rechaza
    // completo sin cambiar nada; con un ticket libre se ejecuta.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 50, 100)));
        engine.fijarSiguienteTicketPruebas(MAXIMO);
        prepararPaso(engine);
        const EstadoSistema antes = engine.obtenerEstado();
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::ESTADO_INVALIDO);
        EXPECT_TRUE(diferencia(antes, engine.obtenerEstado()).empty());
        EXPECT_EQ(engine.obtenerEstado().pidEjecutando, -1);

        engine.fijarSiguienteTicketPruebas(MAXIMO - 1);
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
        EXPECT_EQ(buscar(engine.obtenerEstado(), 1)->ticketReady, MAXIMO - 1);
    }

    // Caso 6 (idle): el salto del reloj hasta un despertar necesita un ticket. Con
    // el contador agotado se rechaza antes de avanzar el reloj ni emitir eventos.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 50, 20, {true, 8, 10})));
        prepararPaso(engine);
        engine.fijarSiguienteTicketPruebas(MAXIMO);  // pasar a WAIT no consume
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
        EXPECT_EQ(buscar(engine.obtenerEstado(), 1)->estado, Estado::WAIT);

        const EstadoSistema antes = engine.obtenerEstado();
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::ESTADO_INVALIDO);
        const EstadoSistema tras = engine.obtenerEstado();
        EXPECT_TRUE(diferencia(antes, tras).empty());
        EXPECT_EQ(tras.tiempoSimuladoMs, 8);  // el reloj no avanzo
        EXPECT_EQ(contarEventos(tras, TipoEvento::CPU_IDLE_INICIO), 0);

        engine.fijarSiguienteTicketPruebas(MAXIMO - 1);
        const ResultadoPaso r = engine.siguientePaso();
        EXPECT_EQ(r.codigo, CodigoResultado::OK);
        EXPECT_FALSE(r.ejecutoCpu);
        const EstadoSistema fin = engine.obtenerEstado();
        EXPECT_EQ(fin.tiempoSimuladoMs, 18);
        EXPECT_EQ(buscar(fin, 1)->ticketReady, MAXIMO - 1);
        EXPECT_EQ(contarEventos(fin, TipoEvento::CPU_IDLE_INICIO), 1);
    }

    // Caso 7: la reserva cuenta los despertares de WAIT que ocurren durante la
    // rafaga. Con dos tickets libres un paso que necesita tres se rechaza.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 50, 100, {true, 5, 25})));
        EXPECT_TRUE(engine.agregarProceso(alta(2, 50, 100, {true, 5, 25})));
        EXPECT_TRUE(engine.agregarProceso(alta(3, 50, 100)));
        prepararPaso(engine);
        engine.fijarSiguienteTicketPruebas(MAXIMO);
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);  // P1 a WAIT
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);  // P2 a WAIT

        // P3 corre 30 ms: P1 y P2 despiertan dentro de la rafaga (t=30 y t=35 < 40)
        // y P3 agota su cuota: hacen falta 3 tickets.
        engine.fijarSiguienteTicketPruebas(MAXIMO - 2);
        const EstadoSistema antes = engine.obtenerEstado();
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::ESTADO_INVALIDO);
        EXPECT_TRUE(diferencia(antes, engine.obtenerEstado()).empty());

        engine.fijarSiguienteTicketPruebas(MAXIMO - 3);
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(buscar(s, 1)->ticketReady, MAXIMO - 3);
        EXPECT_EQ(buscar(s, 2)->ticketReady, MAXIMO - 2);
        EXPECT_EQ(buscar(s, 3)->ticketReady, MAXIMO - 1);
        EXPECT_EQ(invariantesVioladas(s), 0);
    }

    // Caso 8: reiniciar restablece el contador; tras el agotamiento el motor
    // vuelve a admitir procesos.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        engine.fijarSiguienteTicketPruebas(MAXIMO);
        EXPECT_EQ(engine.agregarProceso(alta(1, 50, 50)).codigo,
                  CodigoResultado::ESTADO_INVALIDO);
        EXPECT_TRUE(engine.reiniciar());
        EXPECT_TRUE(engine.agregarProceso(alta(1, 50, 50)));
        EXPECT_EQ(buscar(engine.obtenerEstado(), 1)->ticketReady, 1U);
        EXPECT_THROW(engine.fijarSiguienteTicketPruebas(0), std::invalid_argument);
    }

    return test::finish();
}
