#pragma once

#include "contracts/v2/Configuracion.h"
#include "contracts/v2/EstadoSistema.h"

namespace ConcurrentSimulator::v2 {

// Estos tipos y sus semánticas son una PROPUESTA TÉCNICA pendiente de
// ratificación; no son parte de las 19 decisiones funcionales aprobadas.
enum class CodigoResultado {
    OK,
    ARGUMENTO_INVALIDO,
    PID_DUPLICADO,
    MEMORIA_INSUFICIENTE,
    PID_INEXISTENTE,
    PID_FINALIZADO,
    PID_YA_CANCELADO,
    ESTADO_INVALIDO
};

struct Resultado {
    CodigoResultado codigo{CodigoResultado::OK};
    int pid{-1};
    explicit operator bool() const noexcept { return codigo == CodigoResultado::OK; }
};

struct ResultadoPaso {
    CodigoResultado codigo{CodigoResultado::OK};
    bool avanzo{false};
    bool ejecutoCpu{false};
    std::int64_t tiempoAvanzadoMs{0};
};

// PROPUESTA TÉCNICA, no definida por DP-01–DP-11: los motores implementarían
// esta interfaz común y publicarían la misma semántica observable.
class IExecutionEngine {
public:
    virtual ~IExecutionEngine() = default;
    virtual Resultado agregarProceso(const AltaProceso& proceso) = 0;
    // Semántica propuesta: OK si cancela activo; PID_INEXISTENTE,
    // PID_FINALIZADO o PID_YA_CANCELADO distinguen el estado sin mutarlo.
    // Repetir cancelación es idempotente respecto del estado e historial.
    virtual Resultado cancelarProceso(int pid) = 0;
    // En modo continuo, driver/unidad entre llamadas aún por ratificar.
    virtual Resultado iniciar() = 0;  // Inicia o reanuda.
    virtual Resultado pausar() = 0;
    virtual Resultado reiniciar() = 0;  // Conserva Configuracion.
    virtual ResultadoPaso siguientePaso() = 0;  // Requiere PAUSADO.
    virtual EstadoSistema obtenerEstado() const = 0;
};

}  // namespace ConcurrentSimulator::v2
