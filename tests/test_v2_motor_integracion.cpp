#include "simulation/SimulationEngineV2.h"
#include "tests/TestSupport.h"

#include <algorithm>

using namespace ConcurrentSimulator::v2;

int main() {
    SimulationEngineV2 motor(Configuracion{250});
    EXPECT_TRUE(motor.agregarProceso({1, "pequeno", 12, 30, {true, 10, 5}}));
    EXPECT_TRUE(motor.agregarProceso({2, "mediano", 40, 50, {}}));
    EXPECT_TRUE(motor.agregarProceso({3, "grande", 96, 5, {}}));

    EstadoSistema estado = motor.obtenerEstado();
    EXPECT_EQ(estado.memoriaUsadaBytes, 148);
    EXPECT_EQ(estado.colaReady, (std::vector<int>{3, 2, 1}));
    EXPECT_EQ(estado.procesos[0].prioridad, Prioridad::P1);
    EXPECT_EQ(estado.procesos[1].prioridad, Prioridad::P2);
    EXPECT_EQ(estado.procesos[2].prioridad, Prioridad::P3);

    EXPECT_TRUE(motor.iniciar());
    EXPECT_TRUE(motor.pausar());

    ResultadoPaso paso = motor.siguientePaso();
    EXPECT_TRUE(paso.avanzo);
    EXPECT_TRUE(paso.ejecutoCpu);
    EXPECT_EQ(paso.tiempoAvanzadoMs, 5);
    estado = motor.obtenerEstado();
    EXPECT_EQ(estado.procesos[2].estado, Estado::FINISH);
    EXPECT_EQ(estado.memoriaUsadaBytes, 52);
    EXPECT_EQ(estado.timeline.size(), 1U);

    paso = motor.siguientePaso();
    EXPECT_TRUE(paso.ejecutoCpu);
    EXPECT_EQ(paso.tiempoAvanzadoMs, 30);
    estado = motor.obtenerEstado();
    EXPECT_EQ(estado.procesos[1].tiempoCpuConsumidoMs, 30);
    EXPECT_EQ(estado.procesos[1].estado, Estado::READY);
    EXPECT_EQ(estado.procesos[1].ticketReady, 4U);
    EXPECT_EQ(estado.memoriaUsadaBytes, 52);

    paso = motor.siguientePaso();
    EXPECT_TRUE(paso.ejecutoCpu);
    EXPECT_EQ(paso.tiempoAvanzadoMs, 20);
    estado = motor.obtenerEstado();
    EXPECT_EQ(estado.procesos[1].estado, Estado::FINISH);
    EXPECT_EQ(estado.memoriaUsadaBytes, 12);

    paso = motor.siguientePaso();
    EXPECT_TRUE(paso.ejecutoCpu);
    EXPECT_EQ(paso.tiempoAvanzadoMs, 10);
    estado = motor.obtenerEstado();
    EXPECT_EQ(estado.procesos[0].estado, Estado::WAIT);
    EXPECT_EQ(estado.memoriaUsadaBytes, 12);

    paso = motor.siguientePaso();
    EXPECT_TRUE(paso.avanzo);
    EXPECT_FALSE(paso.ejecutoCpu);
    EXPECT_EQ(paso.tiempoAvanzadoMs, 5);
    estado = motor.obtenerEstado();
    EXPECT_EQ(estado.procesos[0].estado, Estado::READY);

    paso = motor.siguientePaso();
    EXPECT_TRUE(paso.ejecutoCpu);
    EXPECT_EQ(paso.tiempoAvanzadoMs, 20);
    estado = motor.obtenerEstado();
    EXPECT_EQ(estado.procesos[0].estado, Estado::FINISH);
    EXPECT_EQ(estado.memoriaUsadaBytes, 0);
    EXPECT_EQ(estado.tiempoSimuladoMs, 90);
    EXPECT_EQ(estado.timeline.size(), 5U);
    EXPECT_TRUE(std::all_of(estado.timeline.begin(), estado.timeline.end(),
                            [](const BloqueTimeline& bloque) {
                                return bloque.pid() > 0 &&
                                       bloque.finMs() > bloque.inicioMs();
                            }));
    EXPECT_TRUE(!estado.eventos.empty());

    EXPECT_TRUE(motor.reiniciar());
    estado = motor.obtenerEstado();
    EXPECT_EQ(estado.tiempoSimuladoMs, 0);
    EXPECT_EQ(estado.procesos.size(), 0U);
    EXPECT_EQ(estado.memoriaUsadaBytes, 0);
    EXPECT_EQ(estado.eventos.size(), 0U);
    EXPECT_EQ(estado.timeline.size(), 0U);

    return test::finish();
}
