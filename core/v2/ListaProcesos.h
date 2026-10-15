#pragma once

#include "contracts/v2/Proceso.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace ConcurrentSimulator::v2 {

// API provisional del módulo Castilla. Los punteros de buscar() son prestados:
// dejan de ser válidos al borrar ese PID o destruir/mover la lista.
class ListaProcesos {
public:
    ListaProcesos() = default;
    ~ListaProcesos();

    ListaProcesos(const ListaProcesos& otra);
    ListaProcesos& operator=(const ListaProcesos& otra);
    ListaProcesos(ListaProcesos&& otra) noexcept;
    ListaProcesos& operator=(ListaProcesos&& otra) noexcept;

    void intercambiar(ListaProcesos& otra) noexcept;
    bool agregar(const Proceso& proceso);
    bool eliminar(int pid) noexcept;

    const Proceso* buscar(int pid) const noexcept;
    std::vector<Proceso> obtenerTodos() const;
    std::vector<Proceso> obtenerPorEstado(Estado estado) const;

    // Aplica las transiciones operativas v2; repetir el estado actual es
    // idempotente y FINISH/CANCEL no admiten salidas.
    bool actualizarEstado(int pid, Estado estado) noexcept;
    bool actualizarPrioridad(int pid, Prioridad prioridad) noexcept;
    // CPU consumida solo avanza y CPU restante solo disminuye durante la vida
    // del proceso en la lista; una instancia nueva comienza en cero.
    bool actualizarCpu(int pid, std::int64_t consumidoMs,
                       std::int64_t restanteMs) noexcept;
    bool actualizarTiempoEspera(int pid, std::int64_t esperaMs) noexcept;
    // El Scheduler asigna tickets globales; cero significa sin asignar.
    // Este setter local permite conservar o avanzar el ticket, nunca volver.
    bool actualizarTicketReady(int pid, std::uint64_t ticket) noexcept;

    // Clasifica la población activa y persiste el lote en los nodos. No decide
    // cuándo invocarse ni cambia CPU/cuota fijada del turno RUN.
    bool reclasificarPrioridades();

    bool vacia() const noexcept;
    std::size_t cantidad() const noexcept;

private:
    struct Nodo {
        explicit Nodo(const Proceso& valor) : proceso(valor) {}
        Proceso proceso;
        std::unique_ptr<Nodo> siguiente;
    };

    void limpiar() noexcept;
    struct Nodo* buscarNodo(int pid) noexcept;

    std::unique_ptr<Nodo> primero_;
    Nodo* ultimo_{nullptr};
    std::size_t cantidad_{0};
};

}  // namespace ConcurrentSimulator::v2
