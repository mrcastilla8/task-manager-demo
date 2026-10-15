#include "core/v2/AdmisorProcesos.h"

namespace ConcurrentSimulator::v2 {
namespace {

bool insertarEnLista(ListaProcesos& procesos, const Proceso& proceso) {
    return procesos.agregar(proceso);
}

class RollbackReserva {
public:
    RollbackReserva(Memoria& memoria, int pid) noexcept
        : memoria_(memoria), pid_(pid) {}
    ~RollbackReserva() {
        if (activa_) {
            (void)memoria_.liberar(pid_);
        }
    }

    void confirmar() noexcept { activa_ = false; }

private:
    Memoria& memoria_;
    int pid_;
    bool activa_{true};
};

}  // namespace

ResultadoAdmision admitirProceso(const AltaProceso& alta,
                                 ListaProcesos& procesos,
                                 Memoria& memoria) {
    return detail::admitirProcesoConInsercion(
        alta, procesos, memoria, &insertarEnLista);
}

ResultadoAdmision detail::admitirProcesoConInsercion(
    const AltaProceso& alta, ListaProcesos& procesos, Memoria& memoria,
    InsertarProceso insertar) {
    if (!validarAltaProceso(alta)) {
        return ResultadoAdmision::ALTA_INVALIDA;
    }
    if (procesos.buscar(alta.pid) != nullptr || memoria.estaReservado(alta.pid)) {
        return ResultadoAdmision::PID_DUPLICADO;
    }

    Proceso proceso = construirProcesoInicial(alta);
    if (!memoria.puedeReservar(alta.pid, alta.tamanioBytes)) {
        return ResultadoAdmision::MEMORIA_INSUFICIENTE;
    }
    if (!memoria.reservar(alta.pid, alta.tamanioBytes)) {
        return memoria.estaReservado(alta.pid) ||
                       procesos.buscar(alta.pid) != nullptr
                   ? ResultadoAdmision::PID_DUPLICADO
                   : ResultadoAdmision::MEMORIA_INSUFICIENTE;
    }

    RollbackReserva rollback(memoria, alta.pid);
    bool insertado = false;
    try {
        insertado = insertar != nullptr && insertar(procesos, proceso);
    } catch (...) {
        (void)procesos.eliminar(alta.pid);
        throw;
    }
    if (!insertado) {
        (void)procesos.eliminar(alta.pid);
        return ResultadoAdmision::INSERCION_RECHAZADA;
    }
    rollback.confirmar();
    return ResultadoAdmision::ACEPTADO;
}

}  // namespace ConcurrentSimulator::v2
