#include "core/v2/ListaProcesos.h"
#include "core/v2/Proceso.h"

#include <utility>

namespace ConcurrentSimulator::v2 {
namespace {

bool esActivo(Estado estado) noexcept {
    return estado == Estado::READY || estado == Estado::RUN ||
           estado == Estado::WAIT;
}

bool mismaVersionSinPrioridad(const Proceso& a, const Proceso& b) noexcept {
    return a.pid == b.pid && a.nombre == b.nombre &&
           a.tamanioBytes == b.tamanioBytes &&
           a.tiempoCpuTotalMs == b.tiempoCpuTotalMs &&
           a.tiempoCpuRestanteMs == b.tiempoCpuRestanteMs &&
           a.tiempoCpuConsumidoMs == b.tiempoCpuConsumidoMs &&
           a.tiempoEsperaMs == b.tiempoEsperaMs &&
           a.ticketReady == b.ticketReady && a.estado == b.estado &&
           a.operacionIO.habilitada == b.operacionIO.habilitada &&
           a.operacionIO.despuesDeCpuMs == b.operacionIO.despuesDeCpuMs &&
           a.operacionIO.duracionEsperaMs == b.operacionIO.duracionEsperaMs;
}

bool prioridadValida(Prioridad prioridad) noexcept {
    return prioridad == Prioridad::P1 || prioridad == Prioridad::P2 ||
           prioridad == Prioridad::P3;
}

}  // namespace

ListaProcesos::~ListaProcesos() { limpiar(); }

void ListaProcesos::limpiar() noexcept {
    while (primero_ != nullptr) {
        std::unique_ptr<Nodo> siguiente = std::move(primero_->siguiente);
        primero_ = std::move(siguiente);
    }
    ultimo_ = nullptr;
    cantidad_ = 0;
}

ListaProcesos::ListaProcesos(const ListaProcesos& otra) {
    for (const Nodo* nodo = otra.primero_.get(); nodo != nullptr;
         nodo = nodo->siguiente.get()) {
        (void)agregar(nodo->proceso);
    }
}

ListaProcesos& ListaProcesos::operator=(const ListaProcesos& otra) {
    if (this != &otra) {
        ListaProcesos copia(otra);
        intercambiar(copia);
    }
    return *this;
}

ListaProcesos::ListaProcesos(ListaProcesos&& otra) noexcept
    : primero_(std::move(otra.primero_)),
      ultimo_(otra.ultimo_),
      cantidad_(otra.cantidad_) {
    otra.ultimo_ = nullptr;
    otra.cantidad_ = 0;
}

ListaProcesos& ListaProcesos::operator=(ListaProcesos&& otra) noexcept {
    if (this != &otra) {
        limpiar();
        primero_ = std::move(otra.primero_);
        ultimo_ = otra.ultimo_;
        cantidad_ = otra.cantidad_;
        otra.ultimo_ = nullptr;
        otra.cantidad_ = 0;
    }
    return *this;
}

void ListaProcesos::intercambiar(ListaProcesos& otra) noexcept {
    primero_.swap(otra.primero_);
    std::swap(ultimo_, otra.ultimo_);
    std::swap(cantidad_, otra.cantidad_);
}

bool ListaProcesos::agregar(const Proceso& proceso) {
    if (!procesoConsistente(proceso) || buscar(proceso.pid) != nullptr) {
        return false;
    }
    auto nuevo = std::make_unique<Nodo>(proceso);
    Nodo* nuevoUltimo = nuevo.get();
    if (ultimo_ == nullptr) {
        primero_ = std::move(nuevo);
    } else {
        ultimo_->siguiente = std::move(nuevo);
    }
    ultimo_ = nuevoUltimo;
    ++cantidad_;
    return true;
}

bool ListaProcesos::eliminar(int pid) noexcept {
    std::unique_ptr<Nodo>* enlace = &primero_;
    Nodo* anterior = nullptr;
    while (*enlace != nullptr && (*enlace)->proceso.pid != pid) {
        anterior = enlace->get();
        enlace = &((*enlace)->siguiente);
    }
    if (*enlace == nullptr) {
        return false;
    }
    const bool eraUltimo = enlace->get() == ultimo_;
    std::unique_ptr<Nodo> eliminado = std::move(*enlace);
    *enlace = std::move(eliminado->siguiente);
    if (eraUltimo) {
        ultimo_ = anterior;
    }
    --cantidad_;
    return true;
}

const Proceso* ListaProcesos::buscar(int pid) const noexcept {
    for (const Nodo* nodo = primero_.get(); nodo != nullptr;
         nodo = nodo->siguiente.get()) {
        if (nodo->proceso.pid == pid) {
            return &nodo->proceso;
        }
    }
    return nullptr;
}

ListaProcesos::Nodo* ListaProcesos::buscarNodo(int pid) noexcept {
    for (Nodo* nodo = primero_.get(); nodo != nullptr;
         nodo = nodo->siguiente.get()) {
        if (nodo->proceso.pid == pid) {
            return nodo;
        }
    }
    return nullptr;
}

bool ListaProcesos::actualizarEstado(int pid, Estado estado) noexcept {
    switch (estado) {
        case Estado::READY:
        case Estado::RUN:
        case Estado::WAIT:
        case Estado::FINISH:
        case Estado::CANCEL:
            break;
        default:
            return false;
    }
    Nodo* nodo = buscarNodo(pid);
    if (nodo == nullptr) {
        return false;
    }
    const Estado actual = nodo->proceso.estado;
    // Repetir el estado actual es inocuo, incluidos los dos estados terminales.
    if (actual == estado) {
        return true;
    }

    // Las transiciones operativas aprobadas para v2. FINISH solo se alcanza
    // desde RUN cuando la CPU ya se agotó; CANCEL está permitido desde todo
    // estado activo. Los terminales no tienen salidas.
    bool permitida = false;
    switch (actual) {
        case Estado::READY:
            permitida = estado == Estado::RUN || estado == Estado::CANCEL;
            break;
        case Estado::RUN:
            permitida = estado == Estado::READY || estado == Estado::WAIT ||
                        estado == Estado::CANCEL ||
                        (estado == Estado::FINISH &&
                         nodo->proceso.tiempoCpuRestanteMs == 0);
            break;
        case Estado::WAIT:
            permitida = estado == Estado::READY || estado == Estado::CANCEL;
            break;
        case Estado::FINISH:
        case Estado::CANCEL:
            return false;
    }
    if (!permitida) {
        return false;
    }
    nodo->proceso.estado = estado;
    return true;
}

bool ListaProcesos::actualizarPrioridad(int pid,
                                        Prioridad prioridad) noexcept {
    if (!prioridadValida(prioridad)) {
        return false;
    }
    Nodo* nodo = buscarNodo(pid);
    if (nodo == nullptr) {
        return false;
    }
    nodo->proceso.prioridad = prioridad;
    return true;
}

bool ListaProcesos::aplicarPrioridades(
    const std::vector<Proceso>& vistaCalculada) {
    if (vistaCalculada.size() != cantidad_) {
        return false;
    }

    // Primera pasada: verificar cobertura única, que la vista siga vigente y
    // que todos los valores sean aplicables. No se escribe en ningún nodo.
    for (std::size_t indice = 0; indice < vistaCalculada.size(); ++indice) {
        const Proceso& candidato = vistaCalculada[indice];
        if (!procesoConsistente(candidato) ||
            !prioridadValida(candidato.prioridad)) {
            return false;
        }
        for (std::size_t anterior = 0; anterior < indice; ++anterior) {
            if (vistaCalculada[anterior].pid == candidato.pid) {
                return false;
            }
        }
        const Proceso* actual = buscar(candidato.pid);
        if (actual == nullptr || !mismaVersionSinPrioridad(*actual, candidato) ||
            (!esActivo(actual->estado) &&
             candidato.prioridad != actual->prioridad)) {
            return false;
        }
    }

    // Segunda pasada: todas las validaciones terminaron, así que el lote se
    // aplica completo y conserva los estados terminales sin cambios.
    for (const Proceso& candidato : vistaCalculada) {
        if (esActivo(candidato.estado)) {
            (void)actualizarPrioridad(candidato.pid, candidato.prioridad);
        }
    }
    return true;
}

bool ListaProcesos::actualizarCpu(int pid, std::int64_t consumidoMs,
                                  std::int64_t restanteMs) noexcept {
    Nodo* nodo = buscarNodo(pid);
    if (nodo == nullptr) {
        return false;
    }
    if (consumidoMs == nodo->proceso.tiempoCpuConsumidoMs &&
        restanteMs == nodo->proceso.tiempoCpuRestanteMs) {
        return true;
    }
    if (nodo->proceso.estado != Estado::RUN || consumidoMs < 0 ||
        restanteMs < 0 ||
        consumidoMs > nodo->proceso.tiempoCpuTotalMs ||
        restanteMs != nodo->proceso.tiempoCpuTotalMs - consumidoMs ||
        consumidoMs < nodo->proceso.tiempoCpuConsumidoMs ||
        restanteMs > nodo->proceso.tiempoCpuRestanteMs) {
        return false;
    }
    nodo->proceso.tiempoCpuConsumidoMs = consumidoMs;
    nodo->proceso.tiempoCpuRestanteMs = restanteMs;
    return true;
}

bool ListaProcesos::actualizarTiempoEspera(int pid,
                                           std::int64_t esperaMs) noexcept {
    Nodo* nodo = buscarNodo(pid);
    if (nodo == nullptr || esperaMs < 0) {
        return false;
    }
    nodo->proceso.tiempoEsperaMs = esperaMs;
    return true;
}

bool ListaProcesos::actualizarTicketReady(int pid,
                                          std::uint64_t ticket) noexcept {
    Nodo* nodo = buscarNodo(pid);
    if (nodo == nullptr || ticket < nodo->proceso.ticketReady) {
        return false;
    }
    nodo->proceso.ticketReady = ticket;
    return true;
}

std::vector<Proceso> ListaProcesos::obtenerTodos() const {
    std::vector<Proceso> resultado;
    resultado.reserve(cantidad_);
    for (const Nodo* nodo = primero_.get(); nodo != nullptr;
         nodo = nodo->siguiente.get()) {
        resultado.push_back(nodo->proceso);
    }
    return resultado;
}

std::vector<Proceso> ListaProcesos::obtenerPorEstado(Estado estado) const {
    std::vector<Proceso> resultado;
    for (const Nodo* nodo = primero_.get(); nodo != nullptr;
         nodo = nodo->siguiente.get()) {
        if (nodo->proceso.estado == estado) {
            resultado.push_back(nodo->proceso);
        }
    }
    return resultado;
}

bool ListaProcesos::vacia() const noexcept { return primero_ == nullptr; }

std::size_t ListaProcesos::cantidad() const noexcept { return cantidad_; }

}  // namespace ConcurrentSimulator::v2
