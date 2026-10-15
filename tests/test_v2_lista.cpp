#include "core/v2/ListaProcesos.h"
#include "core/v2/Proceso.h"
#include "tests/TestSupport.h"

#include <utility>

using namespace ConcurrentSimulator::v2;

namespace {
Proceso proceso(int pid, Estado estado = Estado::READY) {
    Proceso result = construirProcesoInicial(
        AltaProceso{pid, "p" + std::to_string(pid), pid * 10, 100, {}});
    result.estado = estado;
    return result;
}
}  // namespace

int main() {
    ListaProcesos lista;
    EXPECT_TRUE(lista.vacia());
    EXPECT_EQ(lista.cantidad(), 0U);
    EXPECT_TRUE(lista.buscar(1) == nullptr);
    EXPECT_FALSE(lista.eliminar(1));

    EXPECT_TRUE(lista.agregar(proceso(1)));
    EXPECT_TRUE(lista.agregar(proceso(2, Estado::WAIT)));
    EXPECT_TRUE(lista.agregar(proceso(3)));
    EXPECT_FALSE(lista.agregar(proceso(2)));
    EXPECT_EQ(lista.cantidad(), 3U);
    EXPECT_EQ(lista.buscar(2)->estado, Estado::WAIT);
    EXPECT_TRUE(lista.buscar(99) == nullptr);
    const auto todos = lista.obtenerTodos();
    EXPECT_EQ(todos.size(), 3U);
    EXPECT_EQ(todos[0].pid, 1);
    EXPECT_EQ(todos[1].pid, 2);
    EXPECT_EQ(todos[2].pid, 3);
    EXPECT_EQ(lista.obtenerPorEstado(Estado::READY).size(), 2U);

    EXPECT_TRUE(lista.eliminar(2));  // medio
    EXPECT_TRUE(lista.eliminar(1));  // primero
    EXPECT_EQ(lista.buscar(3)->pid, 3);
    EXPECT_TRUE(lista.agregar(proceso(4)));
    EXPECT_TRUE(lista.eliminar(4));  // último
    EXPECT_TRUE(lista.eliminar(3));  // único restante
    EXPECT_TRUE(lista.vacia());
    EXPECT_FALSE(lista.eliminar(3));

    EXPECT_TRUE(lista.agregar(proceso(10)));
    EXPECT_TRUE(lista.agregar(proceso(11)));
    ListaProcesos copia(lista);
    EXPECT_EQ(copia.cantidad(), 2U);
    copia.buscar(10)->nombre = "copia";
    EXPECT_EQ(lista.buscar(10)->nombre, "p10");
    ListaProcesos asignada;
    asignada.agregar(proceso(99));
    asignada = lista;
    asignada.eliminar(11);
    EXPECT_EQ(lista.cantidad(), 2U);
    EXPECT_EQ(asignada.cantidad(), 1U);
    asignada = asignada;
    EXPECT_EQ(asignada.cantidad(), 1U);

    ListaProcesos movida(std::move(copia));
    EXPECT_EQ(copia.cantidad(), 0U);
    EXPECT_TRUE(copia.vacia());
    EXPECT_EQ(movida.cantidad(), 2U);
    ListaProcesos destino;
    destino.agregar(proceso(50));
    destino = std::move(movida);
    EXPECT_EQ(movida.cantidad(), 0U);
    EXPECT_EQ(destino.cantidad(), 2U);
    EXPECT_TRUE(destino.eliminar(11));
    EXPECT_TRUE(destino.agregar(proceso(12)));
    EXPECT_EQ(destino.buscar(12)->pid, 12);

    return test::finish();
}
