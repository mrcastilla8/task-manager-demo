#include "core/v2/AdmisorProcesos.h"
#include "tests/TestSupport.h"

#include <new>

using namespace ConcurrentSimulator::v2;

namespace {

bool rechazarInsercion(ListaProcesos& lista, const Proceso& proceso) {
    (void)lista.agregar(proceso);
    return false;
}

bool lanzarEnInsercion(ListaProcesos& lista, const Proceso& proceso) {
    (void)lista.agregar(proceso);
    throw std::bad_alloc();
}

}  // namespace

int main() {
    Memoria memoria(1000);
    ListaProcesos procesos;

    EXPECT_EQ(admitirProceso(AltaProceso{1, "A", 600, 100, {}}, procesos,
                             memoria),
              ResultadoAdmision::ACEPTADO);
    EXPECT_EQ(procesos.cantidad(), 1U);
    EXPECT_EQ(memoria.utilizadaBytes(), 600);

    EXPECT_EQ(admitirProceso(AltaProceso{2, "B", 500, 100, {}}, procesos,
                             memoria),
              ResultadoAdmision::MEMORIA_INSUFICIENTE);
    EXPECT_EQ(procesos.cantidad(), 1U);
    EXPECT_TRUE(procesos.buscar(2) == nullptr);
    EXPECT_FALSE(memoria.estaReservado(2));
    EXPECT_EQ(memoria.utilizadaBytes(), 600);
    EXPECT_EQ(memoria.disponibleBytes(), 400);

    EXPECT_EQ(admitirProceso(AltaProceso{1, "duplicado", 50, 100, {}},
                             procesos, memoria),
              ResultadoAdmision::PID_DUPLICADO);
    EXPECT_EQ(memoria.utilizadaBytes(), 600);
    EXPECT_EQ(procesos.cantidad(), 1U);

    EXPECT_EQ(admitirProceso(AltaProceso{3, "", 50, 100, {}}, procesos,
                             memoria),
              ResultadoAdmision::ALTA_INVALIDA);
    EXPECT_EQ(memoria.utilizadaBytes(), 600);
    EXPECT_EQ(procesos.cantidad(), 1U);

    EXPECT_EQ(detail::admitirProcesoConInsercion(
                  AltaProceso{4, "rechazo", 100, 100, {}}, procesos, memoria,
                  &rechazarInsercion),
              ResultadoAdmision::INSERCION_RECHAZADA);
    EXPECT_EQ(memoria.utilizadaBytes(), 600);
    EXPECT_EQ(memoria.disponibleBytes(), 400);
    EXPECT_FALSE(memoria.estaReservado(4));
    EXPECT_TRUE(procesos.buscar(4) == nullptr);
    EXPECT_EQ(procesos.cantidad(), 1U);

    bool excepcionPropagada = false;
    try {
        (void)detail::admitirProcesoConInsercion(
            AltaProceso{5, "excepcion", 100, 100, {}}, procesos, memoria,
            &lanzarEnInsercion);
    } catch (const std::bad_alloc&) {
        excepcionPropagada = true;
    }
    EXPECT_TRUE(excepcionPropagada);
    EXPECT_EQ(memoria.utilizadaBytes(), 600);
    EXPECT_EQ(memoria.disponibleBytes(), 400);
    EXPECT_FALSE(memoria.estaReservado(5));
    EXPECT_TRUE(procesos.buscar(5) == nullptr);
    EXPECT_EQ(procesos.cantidad(), 1U);

    EXPECT_TRUE(memoria.reservar(6, 25));
    EXPECT_EQ(admitirProceso(AltaProceso{6, "preexistente", 20, 100, {}},
                             procesos, memoria),
              ResultadoAdmision::PID_DUPLICADO);
    EXPECT_TRUE(memoria.estaReservado(6));
    EXPECT_EQ(memoria.utilizadaBytes(), 625);

    EXPECT_EQ(admitirProceso(AltaProceso{7, "despues", 100, 100, {}},
                             procesos, memoria),
              ResultadoAdmision::ACEPTADO);
    EXPECT_EQ(procesos.cantidad(), 2U);
    EXPECT_EQ(memoria.utilizadaBytes(), 725);
    EXPECT_EQ(memoria.disponibleBytes(), 275);

    EXPECT_TRUE(memoria.liberar(1));
    EXPECT_TRUE(procesos.eliminar(1));
    EXPECT_FALSE(procesos.vacia());
    EXPECT_EQ(memoria.utilizadaBytes(), 125);

    return test::finish();
}
