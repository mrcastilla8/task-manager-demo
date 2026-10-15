#include "core/v2/Memoria.h"
#include "core/v2/Proceso.h"
#include "tests/TestSupport.h"

#include <cstdint>
#include <limits>
#include <stdexcept>

using namespace ConcurrentSimulator::v2;

int main() {
    EXPECT_THROW(Memoria(-1), std::invalid_argument);
    Memoria memoria(2048);
    EXPECT_TRUE(memoria.reservar(1, 500));
    EXPECT_TRUE(memoria.reservar(2, 1000));
    EXPECT_EQ(memoria.utilizadaBytes(), 1500);
    EXPECT_EQ(memoria.disponibleBytes(), 548);
    EXPECT_TRUE(memoria.liberar(1));
    EXPECT_EQ(memoria.utilizadaBytes(), 1000);
    EXPECT_FALSE(memoria.liberar(1));
    EXPECT_EQ(memoria.utilizadaBytes(), 1000);

    Memoria insuficiente(1000);
    EXPECT_TRUE(insuficiente.reservar(10, 600));
    EXPECT_FALSE(insuficiente.reservar(11, 500));
    EXPECT_EQ(insuficiente.utilizadaBytes(), 600);
    EXPECT_EQ(insuficiente.disponibleBytes(), 400);
    EXPECT_EQ(insuficiente.cantidadAsignaciones(), 1U);
    EXPECT_FALSE(insuficiente.reservar(10, 100));
    EXPECT_FALSE(insuficiente.reservar(12, 0));
    EXPECT_FALSE(insuficiente.reservar(0, 1));
    EXPECT_FALSE(insuficiente.reservar(12, -1));

    Memoria exacta(1000);
    EXPECT_TRUE(exacta.reservar(20, 1000));
    EXPECT_EQ(exacta.disponibleBytes(), 0);
    EXPECT_FALSE(exacta.reservar(21, 1));
    EXPECT_TRUE(exacta.estaReservado(20));

    Memoria espera(100);
    Proceso procesoEnEspera = construirProcesoInicial(
        AltaProceso{30, "en espera", 50, 100, {}});
    procesoEnEspera.estado = Estado::WAIT;
    EXPECT_TRUE(espera.reservar(procesoEnEspera.pid,
                                procesoEnEspera.tamanioBytes));
    EXPECT_TRUE(espera.estaReservado(30));  // WAIT no es una operación de Memoria.
    EXPECT_EQ(espera.utilizadaBytes(), 50);
    EXPECT_FALSE(espera.liberar(999));
    EXPECT_EQ(espera.utilizadaBytes(), 50);

    const auto maximo = (std::numeric_limits<std::int64_t>::max)();
    Memoria extrema(maximo);
    EXPECT_TRUE(extrema.reservar(40, maximo));
    EXPECT_EQ(extrema.utilizadaBytes(), maximo);
    EXPECT_EQ(extrema.disponibleBytes(), 0);
    EXPECT_TRUE(extrema.liberar(40));
    EXPECT_EQ(extrema.utilizadaBytes(), 0);
    EXPECT_EQ(extrema.disponibleBytes(), maximo);

    return test::finish();
}
