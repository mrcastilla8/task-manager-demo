#pragma once

#include "contracts/Proceso.h"

#include <cstddef>
#include <list>
#include <vector>

class ColaProcesos {
public:
    void agregar(const Proceso& proceso);
    bool eliminar(int pid);

    Proceso* buscar(int pid);
    const Proceso* buscar(int pid) const;

    std::vector<Proceso> obtenerTodos() const;
    std::vector<Proceso> obtenerPorEstado(Estado estado) const;

    bool vacia() const noexcept;
    std::size_t cantidad() const noexcept;

private:
    std::list<Proceso> procesos_;
};
