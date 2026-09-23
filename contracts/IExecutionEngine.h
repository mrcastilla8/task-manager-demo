#pragma once

#include "contracts/EstadoSistema.h"
#include "contracts/Proceso.h"

#include <cstdint>

class IExecutionEngine {
public:
    virtual ~IExecutionEngine() = default;

    // Lanza std::invalid_argument si el proceso es invalido o su PID ya existe.
    // Lanza std::runtime_error si el proceso valido no puede admitirse.
    virtual void agregarProceso(const Proceso& proceso) = 0;

    // Es idempotente: un PID inexistente o ya cancelado no produce error.
    virtual void cancelarProceso(int pid) = 0;

    // iniciar() tambien reanuda un motor pausado. iniciar() y pausar() son
    // idempotentes cuando el motor ya se encuentra en el estado solicitado.
    virtual void iniciar() = 0;
    virtual void pausar() = 0;

    // Detiene la ejecucion, libera workers y vuelve al estado inicial. Conserva
    // la configuracion (por ejemplo, quantum y capacidad total de memoria).
    virtual void reiniciar() = 0;

    // Ejecuta una transicion significativa. Lanza std::logic_error si el motor
    // no esta pausado.
    virtual void siguientePaso() = 0;

    // El quantum debe ser positivo. Un valor invalido lanza
    // std::invalid_argument. El cambio se aplica a la siguiente asignacion CPU.
    virtual void configurarQuantum(std::int64_t quantumMs) = 0;
    virtual std::int64_t obtenerQuantum() const = 0;

    // Devuelve una fotografia por valor. En motores concurrentes esta operacion
    // debe ser thread-safe.
    virtual EstadoSistema obtenerEstado() const = 0;
};
