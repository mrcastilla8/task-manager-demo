#pragma once

#include <cstdint>
#include <cstddef>
#include <unordered_map>

namespace ConcurrentSimulator::v2 {

// API provisional del módulo Castilla. Solo registra capacidad y asignaciones;
// no modela direcciones físicas, paginación ni transiciones de proceso.
class Memoria {
public:
    explicit Memoria(std::int64_t capacidadBytes);

    bool puedeReservar(int pid, std::int64_t tamanioBytes) const noexcept;
    bool reservar(int pid, std::int64_t tamanioBytes);
    bool liberar(int pid) noexcept;

    std::int64_t capacidadTotalBytes() const noexcept;
    std::int64_t utilizadaBytes() const noexcept;
    std::int64_t disponibleBytes() const noexcept;
    bool estaReservado(int pid) const noexcept;
    std::size_t cantidadAsignaciones() const noexcept;

private:
    std::int64_t capacidadBytes_;
    std::int64_t utilizadaBytes_{0};
    std::unordered_map<int, std::int64_t> asignaciones_;
};

}  // namespace ConcurrentSimulator::v2
