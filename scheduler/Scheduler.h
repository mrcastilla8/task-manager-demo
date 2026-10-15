#pragma once

#include "contracts/Proceso.h"
#include "scheduler/SchedulerConfig.h"

#include <cstdint>
#include <vector>

/**
 * @brief Planificador de CPU lógico por prioridad, FIFO en empates y aging.
 *
 * Responsabilidad de Integrante 2 (Brickzon):
 * - Selecciona qué proceso en estado READY debe recibir el CPU lógico.
 * - Construye la lista ordenada de PIDs de procesos READY para la cola visible.
 * - Aplica envejecimiento (aging) a los procesos READY según tiempo de espera acumulado.
 * - Restablece la prioridad actual y tiempo de espera al asignar CPU a un proceso.
 *
 * Independiente de hilos, GUI, temporizadores del sistema y motores de ejecución.
 */
class Scheduler {
public:
    explicit Scheduler(SchedulerConfig config = {}) noexcept;

    /// Retorna la configuración actual del planificador.
    const SchedulerConfig& configuracion() const noexcept;

    /// Actualiza la configuración del planificador.
    void configurar(const SchedulerConfig& config) noexcept;

    /**
     * @brief Selecciona el PID del proceso READY con mayor prioridad.
     *
     * Criterio determinista:
     * 1. Mayor prioridadActual.
     * 2. Menor ordenLlegada (FIFO entre empates).
     * 3. Menor PID (desempate estricto final).
     *
     * @param procesos Lista completa de procesos del sistema.
     * @return PID del proceso seleccionado, o -1 si no hay procesos en READY.
     */
    int seleccionarSiguiente(const std::vector<Proceso>& procesos) const noexcept;

    /**
     * @brief Aplica envejecimiento (aging) a los procesos en estado READY.
     *
     * Por cada intervalo completo de `agingIntervaloMs` en `tiempoEspera`,
     * incrementa `prioridadActual` en `agingIncremento` sin superar `prioridadMaxima`.
     * Procesos que no estén en READY no sufren aging ni incrementan `tiempoEspera`.
     *
     * @param procesos Vector de procesos a modificar in-place.
     * @param deltaTiempo Milisegundos simulados transcurridos.
     */
    void aplicarAging(std::vector<Proceso>& procesos,
                      std::int64_t deltaTiempo) const noexcept;

    /**
     * @brief Restablece la prioridadActual a prioridadBase y pone tiempoEspera a 0.
     *
     * Invocado cuando un proceso recibe CPU para iniciar su turno.
     */
    void alAsignarCPU(Proceso& proceso) const noexcept;

    /**
     * @brief Devuelve los PIDs de procesos READY ordenados por prioridad de despacho.
     *
     * Criterio de orden:
     * 1. prioridadActual descendente.
     * 2. ordenLlegada ascendente.
     * 3. pid ascendente.
     *
     * @param procesos Lista de procesos del sistema.
     * @return Vector de PIDs de procesos READY en orden de despacho.
     */
    std::vector<int> ordenarColaPrioridad(
        const std::vector<Proceso>& procesos) const;

private:
    SchedulerConfig config_;
};
