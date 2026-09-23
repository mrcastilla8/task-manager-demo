#pragma once

#include "contracts/Proceso.h"

#include <unordered_map>

class Memoria {
public:
    explicit Memoria(int capacidadTotal);

    bool puedeCargar(const Proceso& proceso) const noexcept;
    bool cargar(const Proceso& proceso);
    void liberar(int pid) noexcept;

    int obtenerTotal() const noexcept;
    int obtenerUtilizada() const noexcept;
    int obtenerDisponible() const noexcept;
    bool estaCargado(int pid) const noexcept;

private:
    int capacidadTotal_;
    int memoriaUtilizada_{0};
    std::unordered_map<int, int> asignaciones_;
};
