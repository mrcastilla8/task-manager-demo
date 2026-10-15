#include "simulation/SimulationEngineV2.h"
#include "tests/FixturesV2.h"
#include "tests/TestSupport.h"

using namespace ConcurrentSimulator::v2;

int main() {
    SimulationEngineV2 motor({150});
    EXPECT_TRUE(motor.agregarProceso({1, "inicial", 100, 40, {}}));
    EXPECT_EQ(motor.agregarProceso({2, "rechazado", 100, 10, {}}).codigo,
              CodigoResultado::MEMORIA_INSUFICIENTE);
    EXPECT_EQ(motor.obtenerEstado().procesos.size(), 1U);
    EXPECT_TRUE(motor.agregarProceso({3, "nuevo", 50, 40, {}}));
    EXPECT_TRUE(motor.iniciar());
    EXPECT_TRUE(motor.pausar());
    EXPECT_EQ(motor.obtenerEstado().colaReady[0], 1);
    const std::uint64_t ticketInicial = motor.obtenerEstado().procesos[0].ticketReady;
    EXPECT_EQ(motor.siguientePaso().tiempoAvanzadoMs, 30);
    EXPECT_EQ(motor.obtenerEstado().procesos[0].tiempoCpuRestanteMs, 10);
    EXPECT_TRUE(motor.obtenerEstado().procesos[0].ticketReady > ticketInicial);
    return test::finish();
}
