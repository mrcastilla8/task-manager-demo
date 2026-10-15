#pragma once

#include "contracts/v2/Configuracion.h"
#include "contracts/v2/EstadoSistema.h"

namespace ConcurrentSimulator::v2 {

enum class CodigoResultado {
    OK,
    ARGUMENTO_INVALIDO,
    PID_DUPLICADO,
    MEMORIA_INSUFICIENTE,
    PID_INEXISTENTE,
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

// Los motores lógico y concurrente implementan esta misma interfaz y
// publican la misma semántica observable. Un turno es una unidad de paso.
class IExecutionEngine {
public:
    virtual ~IExecutionEngine() = default;
    virtual Resultado agregarProceso(const AltaProceso& proceso) = 0;
    // Propuesta API: inexistente/terminal devuelve PID_INEXISTENTE; repetir
    // cancelación no altera estado ni historia.
    virtual Resultado cancelarProceso(int pid) = 0;
    virtual Resultado iniciar() = 0;  // Inicia o reanuda.
    virtual Resultado pausar() = 0;
    virtual Resultado reiniciar() = 0;  // Conserva Configuracion.
    virtual ResultadoPaso siguientePaso() = 0;  // Requiere PAUSADO.
    virtual EstadoSistema obtenerEstado() const = 0;
};

}  // namespace ConcurrentSimulator::v2
