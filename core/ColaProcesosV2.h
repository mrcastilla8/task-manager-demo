#pragma once

#include "contracts/v2/Estado.h"
#include "contracts/v2/Proceso.h"

#include <cstddef>
#include <list>
#include <vector>

namespace ConcurrentSimulator::v2 {

class Scheduler;

/**
 * @brief Estructura de cola/lista de procesos v2 (responsabilidad de Integrante 1 - Castilla).
 *
 * Mantiene la lista de procesos conforme a DP-09 y coordina con el Scheduler (Brickzon):
 * - El calculo de prioridades P1/P2/P3 se delega EXCLUSIVAMENTE al Scheduler (sin duplicacion).
 * - La persistencia de prioridades en la lista no altera estados, CPU restante/consumida,
 *   tickets ni ningun otro campo del proceso.
 * - Valida entradas estrictamente (rechaza tamanioBytes <= 0, PID <= 0, nombre vacio o PID repetido).
 */
class ColaProcesos {
public:
    ColaProcesos() = default;

    /**
     * @brief Agrega un proceso a la lista validando que no exista el PID y que los campos
     *        sean validos (PID > 0, nombre no vacio, tamanioBytes > 0, tiempoCpuTotalMs > 0).
     * @throws std::invalid_argument si el proceso es invalido o ya existe un proceso con el mismo PID.
     */
    void agregar(const Proceso& proceso);

    /**
     * @brief Elimina un proceso por su PID.
     * @return true si fue encontrado y eliminado, false si no existia.
     */
    bool eliminar(int pid);

    /**
     * @brief Busca un proceso por su PID devolviendo puntero mutable o nullptr.
     */
    Proceso* buscar(int pid) noexcept;

    /**
     * @brief Busca un proceso por su PID devolviendo puntero constante o nullptr.
     */
    const Proceso* buscar(int pid) const noexcept;

    /**
     * @brief Obtiene una copia en vector de todos los procesos (incluyendo terminales).
     */
    std::vector<Proceso> obtenerTodos() const;

    /**
     * @brief Obtiene una copia en vector de los procesos en un estado especifico.
     */
    std::vector<Proceso> obtenerPorEstado(Estado estado) const;

    /**
     * @brief Delega al Scheduler el recalculo de prioridades P1/P2/P3 de los procesos activos
     *        en TL = {READY, RUN, WAIT} y las persiste in-place en la lista.
     *        Garantiza que ningun otro dato (estado, tiempos, ticketReady, etc.) sea modificado.
     */
    void recalcularPrioridades(const Scheduler& scheduler);

    bool vacia() const noexcept { return procesos_.empty(); }
    std::size_t cantidad() const noexcept { return procesos_.size(); }

    std::list<Proceso>& procesos() noexcept { return procesos_; }
    const std::list<Proceso>& procesos() const noexcept { return procesos_; }

private:
    std::list<Proceso> procesos_;
};

}  // namespace ConcurrentSimulator::v2
