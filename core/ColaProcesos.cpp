#include "core/ColaProcesos.h"

#include <algorithm>
#include <stdexcept>

void ColaProcesos::agregar(const Proceso& proceso) {
    if (!procesoValido(proceso)) {
        throw std::invalid_argument("No se puede agregar un proceso invalido");
    }
    if (buscar(proceso.pid) != nullptr) {
        throw std::invalid_argument("Ya existe un proceso con el mismo PID");
    }

    procesos_.push_back(proceso);
}

bool ColaProcesos::eliminar(int pid) {
    const auto proceso = std::find_if(
        procesos_.begin(), procesos_.end(),
        [pid](const Proceso& actual) { return actual.pid == pid; });

    if (proceso == procesos_.end()) {
        return false;
    }

    procesos_.erase(proceso);
    return true;
}

Proceso* ColaProcesos::buscar(int pid) {
    const auto proceso = std::find_if(
        procesos_.begin(), procesos_.end(),
        [pid](const Proceso& actual) { return actual.pid == pid; });
    return proceso == procesos_.end() ? nullptr : &*proceso;
}

const Proceso* ColaProcesos::buscar(int pid) const {
    const auto proceso = std::find_if(
        procesos_.cbegin(), procesos_.cend(),
        [pid](const Proceso& actual) { return actual.pid == pid; });
    return proceso == procesos_.cend() ? nullptr : &*proceso;
}

std::vector<Proceso> ColaProcesos::obtenerTodos() const {
    return {procesos_.cbegin(), procesos_.cend()};
}

std::vector<Proceso> ColaProcesos::obtenerPorEstado(Estado estado) const {
    std::vector<Proceso> resultado;
    for (const Proceso& proceso : procesos_) {
        if (proceso.estado == estado) {
            resultado.push_back(proceso);
        }
    }
    return resultado;
}

bool ColaProcesos::vacia() const noexcept {
    return procesos_.empty();
}

std::size_t ColaProcesos::cantidad() const noexcept {
    return procesos_.size();
}
