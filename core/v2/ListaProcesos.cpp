#include "core/v2/ListaProcesos.h"
#include "core/v2/Proceso.h"

#include <utility>

namespace ConcurrentSimulator::v2 {

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
    switch (prioridad) {
        case Prioridad::P1:
        case Prioridad::P2:
        case Prioridad::P3:
            break;
        default:
            return false;
    }
    Nodo* nodo = buscarNodo(pid);
    if (nodo == nullptr) {
        return false;
    }
    nodo->proceso.prioridad = prioridad;
    return true;
}

bool ListaProcesos::actualizarCpu(int pid, std::int64_t consumidoMs,
                                  std::int64_t restanteMs) noexcept {
    Nodo* nodo = buscarNodo(pid);
    if (nodo == nullptr || consumidoMs < 0 || restanteMs < 0 ||
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

bool ListaProcesos::reclasificarPrioridades() {
    std::vector<Proceso> copia = obtenerTodos();
    if (!clasificarPrioridades(copia)) {
        return false;
    }
    Nodo* nodo = primero_.get();
    for (const Proceso& proceso : copia) {
        nodo->proceso.prioridad = proceso.prioridad;
        nodo = nodo->siguiente.get();
    }
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
