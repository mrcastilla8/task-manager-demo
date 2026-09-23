#include "core/Memoria.h"

#include <stdexcept>

Memoria::Memoria(int capacidadTotal) : capacidadTotal_(capacidadTotal) {
    if (capacidadTotal < 0) {
        throw std::invalid_argument("La capacidad de memoria no puede ser negativa");
    }
}

bool Memoria::puedeCargar(const Proceso& proceso) const noexcept {
    if (!procesoValido(proceso) || estaCargado(proceso.pid)) {
        return false;
    }

    return proceso.tamanioBytes <= obtenerDisponible();
}

bool Memoria::cargar(const Proceso& proceso) {
    if (!puedeCargar(proceso)) {
        return false;
    }

    asignaciones_.emplace(proceso.pid, proceso.tamanioBytes);
    memoriaUtilizada_ += proceso.tamanioBytes;
    return true;
}

void Memoria::liberar(int pid) noexcept {
    const auto asignacion = asignaciones_.find(pid);
    if (asignacion == asignaciones_.end()) {
        return;
    }

    memoriaUtilizada_ -= asignacion->second;
    asignaciones_.erase(asignacion);
}

int Memoria::obtenerTotal() const noexcept {
    return capacidadTotal_;
}

int Memoria::obtenerUtilizada() const noexcept {
    return memoriaUtilizada_;
}

int Memoria::obtenerDisponible() const noexcept {
    return capacidadTotal_ - memoriaUtilizada_;
}

bool Memoria::estaCargado(int pid) const noexcept {
    return asignaciones_.find(pid) != asignaciones_.end();
}
