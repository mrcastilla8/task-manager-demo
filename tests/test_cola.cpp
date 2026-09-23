#include "core/ColaProcesos.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

#include <stdexcept>

int main() {
    ColaProcesos cola;
    EXPECT_TRUE(cola.vacia());
    EXPECT_EQ(cola.cantidad(), 0U);

    cola.agregar(crearProceso(1, "A", 100, Estado::READY, 8, 0));
    cola.agregar(crearProceso(2, "B", 200, Estado::RUN, 6, 1));
    cola.agregar(crearProceso(3, "C", 300, Estado::READY, 4, 2));
    EXPECT_FALSE(cola.vacia());
    EXPECT_EQ(cola.cantidad(), 3U);

    Proceso* procesoB = cola.buscar(2);
    EXPECT_TRUE(procesoB != nullptr);
    EXPECT_EQ(procesoB->nombre, "B");
    procesoB->estado = Estado::WAIT;
    EXPECT_EQ(cola.buscar(2)->estado, Estado::WAIT);
    EXPECT_TRUE(cola.buscar(99) == nullptr);

    const ColaProcesos& colaConst = cola;
    const Proceso* procesoA = colaConst.buscar(1);
    EXPECT_TRUE(procesoA != nullptr);
    EXPECT_EQ(procesoA->nombre, "A");
    EXPECT_TRUE(colaConst.buscar(99) == nullptr);

    const auto todos = cola.obtenerTodos();
    EXPECT_EQ(todos.size(), 3U);
    EXPECT_EQ(todos[0].pid, 1);
    EXPECT_EQ(todos[1].pid, 2);
    EXPECT_EQ(todos[2].pid, 3);

    const auto listos = cola.obtenerPorEstado(Estado::READY);
    EXPECT_EQ(listos.size(), 2U);
    EXPECT_EQ(listos[0].pid, 1);
    EXPECT_EQ(listos[1].pid, 3);
    EXPECT_TRUE(cola.obtenerPorEstado(Estado::FINISH).empty());

    EXPECT_THROW(cola.agregar(crearProceso(1, "Duplicado", 100)),
                 std::invalid_argument);
    EXPECT_EQ(cola.cantidad(), 3U);

    Proceso invalido = crearProceso(4, "Invalido", 100);
    invalido.prioridadBase = 20;
    EXPECT_THROW(cola.agregar(invalido), std::invalid_argument);
    EXPECT_EQ(cola.cantidad(), 3U);

    EXPECT_TRUE(cola.eliminar(2));
    EXPECT_TRUE(cola.buscar(2) == nullptr);
    EXPECT_EQ(cola.cantidad(), 2U);
    EXPECT_FALSE(cola.eliminar(2));
    EXPECT_EQ(cola.cantidad(), 2U);
    EXPECT_TRUE(cola.eliminar(1));
    EXPECT_TRUE(cola.eliminar(3));
    EXPECT_TRUE(cola.vacia());

    cola.agregar(crearProceso(4, "D", 400));
    EXPECT_EQ(cola.cantidad(), 1U);
    EXPECT_EQ(cola.buscar(4)->nombre, "D");

    return test::finish();
}
