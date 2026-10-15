#include "core/v2/Memoria.h"

#include <stdexcept>

namespace ConcurrentSimulator::v2 {

Memoria::Memoria(std::int64_t capacidadBytes)
    : capacidadBytes_(capacidadBytes) {
    if (capacidadBytes < 0) {
        throw std::invalid_argument("La capacidad no puede ser negativa");
    }
}

bool Memoria::puedeReservar(int pid, std::int64_t tamanioBytes) const noexcept {
    return pid > 0 && tamanioBytes > 0 && !estaReservado(pid) &&
           tamanioBytes <= disponibleBytes();
}

bool Memoria::reservar(int pid, std::int64_t tamanioBytes) {
    if (!puedeReservar(pid, tamanioBytes)) {
        return false;
    }
    // Inserta primero: si la asignación de memoria lanza, el contador sigue
    // intacto. El incremento queda protegido por tamanio <= disponible.
    const auto insercion = asignaciones_.emplace(pid, tamanioBytes);
    if (!insercion.second) {
        return false;
    }
    utilizadaBytes_ += tamanioBytes;
    return true;
}

bool Memoria::liberar(int pid) noexcept {
    const auto asignacion = asignaciones_.find(pid);
    if (asignacion == asignaciones_.end()) {
        return false;
    }
    utilizadaBytes_ -= asignacion->second;
    asignaciones_.erase(asignacion);
    return true;
}

std::int64_t Memoria::capacidadTotalBytes() const noexcept {
    return capacidadBytes_;
}

std::int64_t Memoria::utilizadaBytes() const noexcept {
    return utilizadaBytes_;
}

std::int64_t Memoria::disponibleBytes() const noexcept {
    return capacidadBytes_ - utilizadaBytes_;
}

bool Memoria::estaReservado(int pid) const noexcept {
    return asignaciones_.find(pid) != asignaciones_.end();
}

std::size_t Memoria::cantidadAsignaciones() const noexcept {
    return asignaciones_.size();
}

}  // namespace ConcurrentSimulator::v2
