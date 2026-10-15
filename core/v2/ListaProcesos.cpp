#include "core/v2/ListaProcesos.h"

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
    if (proceso.pid <= 0 || buscar(proceso.pid) != nullptr) {
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

Proceso* ListaProcesos::buscar(int pid) noexcept {
    return const_cast<Proceso*>(static_cast<const ListaProcesos&>(*this).buscar(pid));
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
