#include "simulation/SimulationEngineV2.h"
#include "tests/FixturesV2.h"
#include "tests/TestSupport.h"

using namespace ConcurrentSimulator::v2;

int main() {
    SimulationEngineV2 motor({100});
    EXPECT_TRUE(motor.agregarProceso({1, "cancelable", 40, 50, {true, 5, 10}}));
    EXPECT_TRUE(motor.iniciar());
    EXPECT_TRUE(motor.pausar());
    EXPECT_EQ(motor.siguientePaso().tiempoAvanzadoMs, 5);
    EXPECT_EQ(motor.obtenerEstado().procesos[0].estado, Estado::WAIT);
    EXPECT_EQ(motor.cancelarProceso(1).codigo, CodigoResultado::OK);
    EXPECT_EQ(motor.obtenerEstado().procesos[0].estado, Estado::CANCEL);
    EXPECT_EQ(motor.obtenerEstado().memoriaUsadaBytes, 0);
    EXPECT_EQ(motor.cancelarProceso(1).codigo, CodigoResultado::PID_YA_CANCELADO);
    EXPECT_EQ(motor.cancelarProceso(9).codigo, CodigoResultado::PID_INEXISTENTE);

    SimulationEngineV2 listo({100});
    EXPECT_TRUE(listo.agregarProceso({2, "listo", 40, 10, {}}));
    EXPECT_EQ(listo.cancelarProceso(2).codigo, CodigoResultado::OK);
    EXPECT_EQ(listo.obtenerEstado().procesos[0].estado, Estado::CANCEL);
    EXPECT_EQ(listo.obtenerEstado().memoriaUsadaBytes, 0);
    return test::finish();
}
