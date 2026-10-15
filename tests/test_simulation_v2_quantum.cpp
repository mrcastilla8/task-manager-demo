#include "simulation/SimulationEngineV2.h"
#include "tests/FixturesV2.h"
#include "tests/TestSupport.h"

using namespace ConcurrentSimulator::v2;

int main() {
    SimulationEngineV2 motor({10'000});
    EXPECT_TRUE(motor.agregarProceso({1, "pequeno", 100, 10, {}}));
    EXPECT_TRUE(motor.agregarProceso({2, "mediano", 200, 20, {}}));
    EXPECT_TRUE(motor.agregarProceso({3, "grande", 300, 50, {true, 30, 100}}));
    EXPECT_TRUE(motor.iniciar());
    EXPECT_TRUE(motor.pausar());

    const ResultadoPaso paso = motor.siguientePaso();
    const EstadoSistema estado = motor.obtenerEstado();
    EXPECT_TRUE(paso.ejecutoCpu);
    EXPECT_EQ(paso.tiempoAvanzadoMs, 30);
    EXPECT_EQ(estado.tiempoSimuladoMs, 30);
    EXPECT_EQ(estado.procesos[2].tiempoCpuRestanteMs, 20);
    EXPECT_EQ(estado.procesos[2].estado, Estado::WAIT);
    EXPECT_EQ(estado.timeline.size(), 1U);
    EXPECT_EQ(estado.timeline[0].pid(), 3);
    EXPECT_EQ(estado.timeline[0].finMs(), 30);

    EXPECT_EQ(motor.siguientePaso().tiempoAvanzadoMs, 20);
    EXPECT_EQ(motor.siguientePaso().tiempoAvanzadoMs, 10);
    EXPECT_EQ(motor.obtenerEstado().procesos[0].estado, Estado::FINISH);
    return test::finish();
}
