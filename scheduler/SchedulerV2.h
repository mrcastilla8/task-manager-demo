#pragma once

#include "contracts/v2/Estado.h"
#include "contracts/v2/Proceso.h"

#include <climits>
#include <cstdint>
#include <list>
#include <stdexcept>
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
 * - Compatible con contenedores std::vector y std::list (de la lista de Castilla).
 * - Ignora y no despacha procesos con ticketReady sin asignar (ticketReady == 0).
 */
class Scheduler {
public:
    static constexpr std::uint64_t TICKET_SIN_ASIGNAR = 0;

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
     * Lanza std::invalid_argument si las entradas son invalidas (tamanos no positivos <= 0,
     * minA > maxB o tamanioBytes fuera del intervalo [minA, maxB]).
     */
    static Prioridad evaluarPrioridad(std::int64_t tamanioBytes,
                                      std::int64_t minA,
                                      std::int64_t maxB);

    /**
     * @brief Recalcula y actualiza in-place las prioridades de los procesos activos
     *        en la poblacion TL = {READY, RUN, WAIT}.
     *
     * Procesos en estado FINISH o CANCEL se excluyen del calculo de extremos (min/max)
     * y su prioridad no se muta (DP-02a). Si TL esta vacia, no realiza cambios.
     * Admisiones rechazadas no alteran TL ni las prioridades existentes (DP-02c).
     *
     * @param procesos Vector de procesos del sistema a reclasificar.
     * @throws std::invalid_argument si algun proceso activo en TL tiene tamano no positivo (<= 0).
     */
    void clasificarPrioridades(std::vector<Proceso>& procesos) const;

    /**
     * @brief Sobrecarga para clasificar prioridades en una lista de procesos (Castilla).
     * @param procesos Lista de procesos a reclasificar in-place.
     * @throws std::invalid_argument si algun proceso activo en TL tiene tamano no positivo (<= 0).
     */
    void clasificarPrioridades(std::list<Proceso>& procesos) const;

    /**
     * @brief Selecciona el PID del proceso en estado READY que debe recibir CPU logico.
     *        Excluye estrictamente procesos con ticket sin asignar (ticketReady == 0).
     *
     * Criterio determinista estricto (DP-05):
     * 1. Mayor prioridad (P3 > P2 > P1).
     * 2. Menor ticketReady (FIFO estricto entre iguales, con ticket > 0).
     * 3. Menor PID (desempate estricto final).
     *
     * @param procesos Lista completa de procesos del sistema.
     * @return PID del proceso READY seleccionado, o -1 si no hay procesos READY despachables.
     */
    int seleccionarSiguiente(const std::vector<Proceso>& procesos) const noexcept;
    int seleccionarSiguiente(const std::list<Proceso>& procesos) const noexcept;

    /**
     * @brief Devuelve los PIDs de los procesos en estado READY ordenados segun el
     *        orden de despacho efectivo para la cola visual de la GUI.
     *        Excluye estrictamente procesos con ticketReady == 0.
     *
     * Sigue exactamente el mismo criterio de ordenacion que `seleccionarSiguiente`.
     *
     * @param procesos Lista completa de procesos del sistema.
     * @return Vector con PIDs de procesos READY en orden de despacho.
     */
    std::vector<int> ordenarColaPrioridad(const std::vector<Proceso>& procesos) const;
    std::vector<int> ordenarColaPrioridad(const std::list<Proceso>& procesos) const;
};

/**
 * @brief Asignador defensivo de tickets estrictamente crecientes para la cola READY (DP-05).
 *        Maneja control de desbordamiento (overflow de uint64_t) conforme a la propuesta tecnica 11.
 */
class AsignadorTickets {
public:
    explicit AsignadorTickets(std::uint64_t inicial = 1) : siguiente_(inicial) {
        if (inicial == Scheduler::TICKET_SIN_ASIGNAR) {
            throw std::invalid_argument("AsignadorTickets: el ticket inicial debe ser mayor a 0");
        }
    }

    /**
     * @brief Asigna el siguiente ticket estrictamente creciente.
     * @throws std::overflow_error si se alcanza el limite maximo de uint64_t (Propuesta 11).
     */
    std::uint64_t asignar() {
        if (siguiente_ == UINT64_MAX) {
            throw std::overflow_error("AsignadorTickets: desbordamiento de tickets READY (uint64_t agotado)");
        }
        return siguiente_++;
    }

    /**
     * @brief Asigna y actualiza directamente el ticket de un proceso que ingresa a READY.
     */
    void renovarTicket(Proceso& proceso) {
        proceso.ticketReady = asignar();
    }

    std::uint64_t proximo() const noexcept { return siguiente_; }
    void reiniciar(std::uint64_t valor = 1) noexcept { siguiente_ = valor; }

private:
    std::uint64_t siguiente_{1};
};

}  // namespace ConcurrentSimulator::v2
