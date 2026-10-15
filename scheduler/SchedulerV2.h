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
     *
     * Criterio determinista estricto (DP-05):
     * 1. Mayor prioridad (P3 > P2 > P1).
     * 2. Menor ticketReady (FIFO estricto entre iguales, con ticket > 0).
     * 3. Menor PID (desempate estricto final).
     *
     * @param procesos Lista completa de procesos del sistema.
     * @return PID del proceso READY seleccionado, o -1 si la cola esta vacia (sin procesos READY).
     * @throws std::logic_error si existen procesos en estado READY con ticket sin asignar (0).
     *         Esto garantiza que una falta de ticket no se confunda con una cola vacia.
     */
    int seleccionarSiguiente(const std::vector<Proceso>& procesos) const;
    int seleccionarSiguiente(const std::list<Proceso>& procesos) const;

    /**
     * @brief Devuelve los PIDs de los procesos en estado READY ordenados segun el
     *        orden de despacho efectivo para la cola visual de la GUI.
     *
     * Sigue exactamente el mismo criterio de ordenacion que `seleccionarSiguiente`.
     *
     * @param procesos Lista completa de procesos del sistema.
     * @return Vector con PIDs de procesos READY en orden de despacho.
     * @throws std::logic_error si existen procesos en estado READY con ticket sin asignar (0).
     */
    std::vector<int> ordenarColaPrioridad(const std::vector<Proceso>& procesos) const;
    std::vector<int> ordenarColaPrioridad(const std::list<Proceso>& procesos) const;

    /**
     * @brief Indica si existen procesos en estado READY con ticket sin asignar (ticketReady == 0).
     *        Permite a los motores de ejecucion validar defensivamente la cola antes del despacho.
     */
    bool hayReadySinTicket(const std::vector<Proceso>& procesos) const noexcept;
    bool hayReadySinTicket(const std::list<Proceso>& procesos) const noexcept;

    /**
     * @brief Cuenta la cantidad total de procesos en estado READY (con o sin ticket).
     */
    std::size_t contarReady(const std::vector<Proceso>& procesos) const noexcept;
    std::size_t contarReady(const std::list<Proceso>& procesos) const noexcept;

    /**
     * @brief Cuenta la cantidad de procesos en estado READY con ticket valido (ticketReady > 0).
     */
    std::size_t contarReadyValidos(const std::vector<Proceso>& procesos) const noexcept;
    std::size_t contarReadyValidos(const std::list<Proceso>& procesos) const noexcept;
};

/**
 * @brief Asignador defensivo de tickets estrictamente crecientes para la cola READY (DP-05).
 *        Maneja control de desbordamiento (overflow de uint64_t) conforme a la propuesta tecnica 11.
 */
class AsignadorTickets {
public:
    explicit AsignadorTickets(std::uint64_t inicial = 1) : siguiente_(inicial) {
        if (inicial == Scheduler::TICKET_SIN_ASIGNAR) {
            throw std::invalid_argument("AsignadorTickets: el ticket inicial no puede ser 0 (TICKET_SIN_ASIGNAR)");
        }
    }

    /**
     * @brief Asigna el siguiente ticket estrictamente creciente.
     *
     * POLITICA PROVISIONAL DE OVERFLOW (DP-05 / Propuesta Tecnica 11):
     * La coordinacion del equipo tiene pendiente de ratificacion la politica definitiva ante
     * el agotamiento de uint64_t (elegir entre lanzar error de limite o renumerar de forma
     * estable la cola READY en un punto pausado).
     * De forma PROVISIONAL y defensiva, esta implementacion lanza std::overflow_error para
     * impedir desbordamientos silenciosos (wrap-around) que corromperian el orden FIFO.
     *
     * @throws std::overflow_error si se alcanza el limite maximo de uint64_t.
     */
    std::uint64_t asignar() {
        if (siguiente_ == UINT64_MAX) {
            throw std::overflow_error(
                "AsignadorTickets: desbordamiento de tickets READY (uint64_t agotado) "
                "[politica provisional pendiente de ratificacion]");
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

    /**
     * @brief Reinicia el contador de tickets a un valor inicial (por defecto 1).
     *        PRECAUCION: Solo debe invocarse al reiniciar una sesion completa de simulacion
     *        (cuando la lista de procesos ha sido vaciada), para evitar emitir tickets
     *        duplicados a procesos que ya se encuentren en el sistema.
     * @param valor Valor inicial de reinicio (debe ser > 0).
     * @throws std::invalid_argument si valor es 0 (TICKET_SIN_ASIGNAR).
     */
    void reiniciar(std::uint64_t valor = 1) {
        if (valor == Scheduler::TICKET_SIN_ASIGNAR) {
            throw std::invalid_argument(
                "AsignadorTickets::reiniciar: el ticket inicial no puede ser 0 (TICKET_SIN_ASIGNAR)");
        }
        siguiente_ = valor;
    }

private:
    std::uint64_t siguiente_{1};
};

}  // namespace ConcurrentSimulator::v2
