#include "simulation/SimulationEngineV2.h"
#include "tests/FixturesV2.h"
#include "tests/TestSupport.h"

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
    return test::finish();
}
