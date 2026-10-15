#include "simulation/SimulationEngineV2.h"
#include "tests/FixturesV2.h"
#include "tests/TestSupport.h"

using namespace ConcurrentSimulator::v2;

int main() {
    SimulationEngineV2 motor({100});
    EXPECT_TRUE(motor.agregarProceso({1, "corto", 40, 7, {}}));
    EXPECT_TRUE(motor.iniciar());
    EXPECT_TRUE(motor.pausar());
    EXPECT_EQ(motor.siguientePaso().tiempoAvanzadoMs, 7);
    const EstadoSistema estado = motor.obtenerEstado();
    EXPECT_EQ(estado.procesos[0].estado, Estado::FINISH);
    EXPECT_EQ(estado.memoriaUsadaBytes, 0);
    EXPECT_EQ(estado.estadoEjecucion, EstadoEjecucion::FINALIZADO);
    EXPECT_EQ(estado.timeline[0].inicioMs(), 0);
    EXPECT_EQ(estado.timeline[0].finMs(), 7);
    EXPECT_TRUE(motor.agregarProceso({2, "nuevo", 40, 4, {}}));
    EXPECT_EQ(motor.obtenerEstado().estadoEjecucion, EstadoEjecucion::DETENIDO);
    EXPECT_TRUE(motor.iniciar());
    EXPECT_TRUE(motor.pausar());
    EXPECT_EQ(motor.siguientePaso().tiempoAvanzadoMs, 4);
    EXPECT_EQ(motor.obtenerEstado().procesos[1].estado, Estado::FINISH);
    return test::finish();
}
