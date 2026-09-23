#include "core/Memoria.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

#include <limits>
#include <stdexcept>

int main() {
    EXPECT_THROW(Memoria(-1), std::invalid_argument);

    Memoria memoria(2048);
    EXPECT_EQ(memoria.obtenerTotal(), 2048);
    EXPECT_EQ(memoria.obtenerUtilizada(), 0);
    EXPECT_EQ(memoria.obtenerDisponible(), 2048);

    const Proceso procesoA = crearProceso(1, "A", 500);
    const Proceso procesoB = crearProceso(2, "B", 1000);
    const Proceso procesoC = crearProceso(3, "C", 548);
    EXPECT_TRUE(memoria.puedeCargar(procesoA));
    EXPECT_TRUE(memoria.cargar(procesoA));
    EXPECT_TRUE(memoria.cargar(procesoB));
    EXPECT_EQ(memoria.obtenerUtilizada(), 1500);
    EXPECT_EQ(memoria.obtenerDisponible(), 548);
    EXPECT_TRUE(memoria.cargar(procesoC));
    EXPECT_EQ(memoria.obtenerUtilizada(), 2048);
    EXPECT_EQ(memoria.obtenerDisponible(), 0);

    const Proceso procesoD = crearProceso(4, "D", 1);
    EXPECT_FALSE(memoria.puedeCargar(procesoD));
    EXPECT_FALSE(memoria.cargar(procesoD));
    EXPECT_EQ(memoria.obtenerUtilizada(), 2048);

    EXPECT_FALSE(memoria.cargar(procesoA));
    EXPECT_EQ(memoria.obtenerUtilizada(), 2048);
    EXPECT_TRUE(memoria.estaCargado(1));

    memoria.liberar(2);
    EXPECT_FALSE(memoria.estaCargado(2));
    EXPECT_EQ(memoria.obtenerUtilizada(), 1048);
    EXPECT_EQ(memoria.obtenerDisponible(), 1000);
    EXPECT_TRUE(memoria.cargar(crearProceso(5, "E", 1000)));
    EXPECT_EQ(memoria.obtenerDisponible(), 0);

    memoria.liberar(999);
    EXPECT_EQ(memoria.obtenerUtilizada(), 2048);
    memoria.liberar(999);
    EXPECT_EQ(memoria.obtenerUtilizada(), 2048);

    Memoria vacia(0);
    EXPECT_EQ(vacia.obtenerDisponible(), 0);
    EXPECT_FALSE(vacia.cargar(crearProceso(1, "A", 1)));

    Memoria grande(std::numeric_limits<int>::max());
    EXPECT_TRUE(grande.cargar(
        crearProceso(1, "Grande", std::numeric_limits<int>::max())));
    EXPECT_EQ(grande.obtenerDisponible(), 0);
    grande.liberar(1);
    EXPECT_EQ(grande.obtenerUtilizada(), 0);

    Proceso tamanioInvalido = crearProceso(8, "Invalido", 10);
    tamanioInvalido.tamanioBytes = 0;
    EXPECT_FALSE(memoria.cargar(tamanioInvalido));

    return test::finish();
}
