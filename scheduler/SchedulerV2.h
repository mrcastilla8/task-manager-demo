#pragma once

#include "contracts/v2/Estado.h"
#include "contracts/v2/Proceso.h"

#include <cstdint>
#include <vector>

namespace ConcurrentSimulator::v2 {

/**
 * @brief Planificador de CPU logico v2 conforme a DP-01, DP-02, DP-03, DP-04, DP-05 y DP-10.
 *
 * Responsabilidad de Integrante 2 (Brickzon):
 * - Clasifica dinamicamente prioridades automaticas P1, P2 y P3 a partir de los tamanos
 *   de los procesos en la poblacion activa TL = {READY, RUN, WAIT} (DP-01, DP-02, DP-03).
 * - Selecciona el proceso READY a despachar mediante regla determinista compartida:
 *   prioridad descendente (P3 > P2 > P1), ticketReady ascendente (FIFO) y PID ascendente (DP-05).
 * - Ordena la cola visible de procesos READY para la interfaz grafica.
 * - Sin envejecimiento (aging) activo (DP-10).
 *
 * Disenado para ser compartido identicamente por el motor logico y el motor concurrente (DP-11b).
 */
class Scheduler {
public:
    Scheduler() noexcept = default;

    /**
     * @brief Evalua la clase de prioridad (P1, P2 o P3) para un tamano dado en funcion
     *        de los extremos A (minimo) y B (maximo) de la poblacion activa TL.
     *
     * Regla estricta DP-03:
     * - Si A == B: retorna P3.
     * - D = B - A.
     * - Si 3 * (S - A) < D: P1.
     * - Si no, si 3 * (S - A) < 2 * D: P2.
     * - En otro caso: P3.
     *
     * Implementacion exacta en int64_t con prevencion estricta de desbordamiento (overflow).
     */
    static Prioridad evaluarPrioridad(std::int64_t tamanioBytes,
                                      std::int64_t minA,
                                      std::int64_t maxB) noexcept;

    /**
     * @brief Recalcula y actualiza in-place las prioridades de los procesos activos
     *        en la poblacion TL = {READY, RUN, WAIT}.
     *
     * Procesos en estado FINISH o CANCEL se excluyen del calculo de extremos (min/max)
     * y su prioridad no se muta (DP-02a). Si TL esta vacia, no realiza cambios.
     *
     * @param procesos Vector de procesos del sistema a reclasificar.
     */
    void clasificarPrioridades(std::vector<Proceso>& procesos) const noexcept;

    /**
     * @brief Selecciona el PID del proceso en estado READY que debe recibir CPU logico.
     *
     * Criterio determinista estricto (DP-05):
     * 1. Mayor prioridad (P3 > P2 > P1).
     * 2. Menor ticketReady (FIFO estricto entre iguales).
     * 3. Menor PID (desempate estricto final).
     *
     * @param procesos Lista completa de procesos del sistema.
     * @return PID del proceso READY seleccionado, o -1 si no hay procesos READY.
     */
    int seleccionarSiguiente(const std::vector<Proceso>& procesos) const noexcept;

    /**
     * @brief Devuelve los PIDs de los procesos en estado READY ordenados segun el
     *        orden de despacho efectivo para la cola visual de la GUI.
     *
     * Sigue exactamente el mismo criterio de ordenacion que `seleccionarSiguiente`.
     *
     * @param procesos Lista completa de procesos del sistema.
     * @return Vector con PIDs de procesos READY en orden de despacho.
     */
    std::vector<int> ordenarColaPrioridad(const std::vector<Proceso>& procesos) const;
};

}  // namespace ConcurrentSimulator::v2
