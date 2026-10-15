#include "core/v2/ListaProcesos.h"
#include "core/v2/Memoria.h"
#include "core/v2/Proceso.h"
#include "tests/TestSupport.h"

using namespace ConcurrentSimulator::v2;

int main() {
    Memoria memoria(1000);
    ListaProcesos procesos;

    const Proceso aceptado = construirProcesoInicial(
        AltaProceso{1, "A", 600, 100, {}});
    if (memoria.reservar(aceptado.pid, aceptado.tamanioBytes)) {
        EXPECT_TRUE(procesos.agregar(aceptado));
    }
    EXPECT_EQ(procesos.cantidad(), 1U);
    EXPECT_EQ(memoria.utilizadaBytes(), 600);

    const Proceso rechazado = construirProcesoInicial(
        AltaProceso{2, "B", 500, 100, {}});
    if (memoria.reservar(rechazado.pid, rechazado.tamanioBytes)) {
        EXPECT_TRUE(procesos.agregar(rechazado));
    }
    EXPECT_EQ(procesos.cantidad(), 1U);
    EXPECT_TRUE(procesos.buscar(2) == nullptr);
    EXPECT_FALSE(memoria.estaReservado(2));
    EXPECT_EQ(memoria.utilizadaBytes(), 600);
    EXPECT_EQ(memoria.disponibleBytes(), 400);

    EXPECT_TRUE(memoria.liberar(1));
    EXPECT_TRUE(procesos.eliminar(1));
    EXPECT_TRUE(procesos.vacia());
    EXPECT_EQ(memoria.utilizadaBytes(), 0);

    return test::finish();
}
