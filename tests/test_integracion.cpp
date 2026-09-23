#include "contracts/EstadoSistema.h"
#include "core/ColaProcesos.h"
#include "core/Memoria.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

int main() {
    ColaProcesos procesos;
    Memoria memoria(1024);

    const Proceso procesoA = crearProceso(1, "A", 600);
    const Proceso procesoB = crearProceso(2, "B", 500);

    procesos.agregar(procesoA);
    EXPECT_TRUE(memoria.cargar(procesoA));

    procesos.agregar(procesoB);
    EXPECT_FALSE(memoria.cargar(procesoB));
    EXPECT_EQ(memoria.obtenerUtilizada(), 600);
    EXPECT_EQ(procesos.cantidad(), 2U);

    EXPECT_TRUE(procesos.eliminar(1));
    memoria.liberar(1);
    EXPECT_TRUE(memoria.cargar(procesoB));
    EXPECT_EQ(memoria.obtenerUtilizada(), 500);

    EstadoSistema snapshot;
    snapshot.procesos = procesos.obtenerTodos();
    snapshot.memoriaUsada = memoria.obtenerUtilizada();
    snapshot.memoriaTotal = memoria.obtenerTotal();

    EXPECT_EQ(snapshot.procesos.size(), 1U);
    EXPECT_EQ(snapshot.procesos[0].pid, 2);
    EXPECT_EQ(snapshot.memoriaUsada, 500);
    EXPECT_EQ(snapshot.memoriaTotal, 1024);

    snapshot.procesos[0].nombre = "Copia modificada";
    EXPECT_EQ(procesos.buscar(2)->nombre, "B");

    for (int pid = 3; pid < 20; ++pid) {
        const Proceso temporal = crearProceso(pid, "Temporal", 10);
        procesos.agregar(temporal);
        EXPECT_TRUE(memoria.cargar(temporal));
        EXPECT_TRUE(procesos.eliminar(pid));
        memoria.liberar(pid);
    }
    EXPECT_EQ(procesos.cantidad(), 1U);
    EXPECT_EQ(memoria.obtenerUtilizada(), 500);

    return test::finish();
}
