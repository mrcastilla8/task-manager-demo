#pragma once

#include "contracts/v2/Proceso.h"


namespace ConcurrentSimulator::v2 {

// API provisional del módulo Castilla. No forma parte de contracts/v2 y debe
// coordinarse con Brickzon antes de su adopción como contrato compartido.
bool validarAltaProceso(const AltaProceso& alta) noexcept;
Proceso construirProcesoInicial(const AltaProceso& alta);
bool cpuConsistente(const Proceso& proceso) noexcept;
bool procesoConsistente(const Proceso& proceso) noexcept;

}  // namespace ConcurrentSimulator::v2
