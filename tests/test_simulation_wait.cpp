#include "simulation/SimulationEngineV2.h"
#include "tests/FixturesV2.h"
#include "tests/TestSupport.h"

#include <limits>

using namespace ConcurrentSimulator::v2;

int main() {
    SimulationEngineV2 motor({1'000});
    EXPECT_TRUE(motor.agregarProceso({1, "io", 100, 20, {true, 8, 10}}));
    EXPECT_TRUE(motor.iniciar());
    EXPECT_TRUE(motor.pausar());
    EXPECT_EQ(motor.siguientePaso().tiempoAvanzadoMs, 8);
    EXPECT_EQ(motor.obtenerEstado().procesos[0].estado, Estado::WAIT);
    const ResultadoPaso idle = motor.siguientePaso();
    const EstadoSistema despierto = motor.obtenerEstado();
    EXPECT_FALSE(idle.ejecutoCpu);
    EXPECT_EQ(idle.tiempoAvanzadoMs, 10);
    EXPECT_EQ(despierto.tiempoSimuladoMs, 18);
    EXPECT_EQ(despierto.procesos[0].estado, Estado::READY);
    EXPECT_EQ(despierto.timeline.size(), 1U);
    EXPECT_EQ(despierto.timeline[0].finMs(), 8);
    EXPECT_EQ(motor.siguientePaso().tiempoAvanzadoMs, 12);
    EXPECT_EQ(motor.obtenerEstado().procesos[0].estado, Estado::FINISH);

    SimulationEngineV2 duranteCpu({1'000});
    EXPECT_TRUE(duranteCpu.agregarProceso({1, "espera", 100, 20, {true, 5, 4}}));
    EXPECT_TRUE(duranteCpu.agregarProceso({2, "cpu", 100, 15, {}}));
    EXPECT_TRUE(duranteCpu.iniciar());
    EXPECT_TRUE(duranteCpu.pausar());
    EXPECT_EQ(duranteCpu.siguientePaso().tiempoAvanzadoMs, 5);
    const std::uint64_t ticketEspera = duranteCpu.obtenerEstado().procesos[0].ticketReady;
    EXPECT_EQ(duranteCpu.siguientePaso().tiempoAvanzadoMs, 15);
    const EstadoSistema trasDespertar = duranteCpu.obtenerEstado();
    EXPECT_EQ(trasDespertar.procesos[0].estado, Estado::READY);
    EXPECT_TRUE(trasDespertar.procesos[0].ticketReady > ticketEspera);
    EXPECT_EQ(trasDespertar.colaReady[0], 1);
    EXPECT_EQ(trasDespertar.timeline.size(), 2U);
    EXPECT_EQ(trasDespertar.timeline[1].inicioMs(), 5);
    EXPECT_EQ(trasDespertar.timeline[1].finMs(), 20);

    SimulationClockV2 reloj;
    EXPECT_TRUE(reloj.puedeAvanzar(std::numeric_limits<std::int64_t>::max()));
    reloj.avanzar(std::numeric_limits<std::int64_t>::max());
    EXPECT_FALSE(reloj.puedeAvanzar(1));
    reloj.avanzar(1);
    EXPECT_EQ(reloj.ahoraMs(), std::numeric_limits<std::int64_t>::max());
    return test::finish();
}
