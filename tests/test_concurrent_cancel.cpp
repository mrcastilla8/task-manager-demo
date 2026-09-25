#include "concurrent/ConcurrentEngine.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

#include <algorithm>

int main() {
    ConcurrentEngine engine(2048);

    Proceso p1 = crearProceso(1, "A", 300, Estado::READY, 5);
    p1.tiempoTotal = 1000;
    p1.tiempoRestante = 1000;

    Proceso p2 = crearProceso(2, "B", 400, Estado::READY, 7);
    p2.tiempoTotal = 1000;
    p2.tiempoRestante = 1000;

    engine.agregarProceso(p1);
    engine.agregarProceso(p2);
    EXPECT_EQ(engine.memoriaUsada(), 700);

    // Caso 1: Cancelar un proceso que no existe (idempotente)
    engine.cancelarProceso(999);
    EXPECT_EQ(engine.memoriaUsada(), 700);

    // Caso 2: Cancelar proceso en READY
    engine.cancelarProceso(1);
    EXPECT_EQ(engine.memoriaUsada(), 400); // se libera memoria de p1

    EstadoSistema s1 = engine.obtenerEstado();
    const auto itP1 = std::find_if(s1.procesos.begin(), s1.procesos.end(),
                                   [](const Proceso& p) { return p.pid == 1; });
    EXPECT_TRUE(itP1 != s1.procesos.end());
    EXPECT_EQ(itP1->estado, Estado::CANCEL);

    // Caso 3: Cancelar nuevamente el mismo proceso ya cancelado (idempotente)
    engine.cancelarProceso(1);
    EXPECT_EQ(engine.memoriaUsada(), 400);

    // Caso 4: Siguiente paso solo ejecuta P2
    engine.pausar();
    engine.siguientePaso();
    EstadoSistema s2 = engine.obtenerEstado();
    EXPECT_EQ(s2.timeline.size(), 1U);
    EXPECT_EQ(s2.timeline[0].pid, 2);

    // Caso 5: Cancelar P2
    engine.cancelarProceso(2);
    EXPECT_EQ(engine.memoriaUsada(), 0);
    EXPECT_EQ(engine.obtenerEstado().estadoEjecucion, EstadoEjecucion::FINALIZADO);

    return test::finish();
}
