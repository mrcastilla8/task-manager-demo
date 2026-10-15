#pragma once

#include "contracts/v2/Proceso.h"

#include <vector>

namespace ConcurrentSimulator::v2 {

// API provisional del módulo Castilla. No forma parte de contracts/v2 y debe
// coordinarse con Brickzon antes de su adopción como contrato compartido.
bool validarAltaProceso(const AltaProceso& alta) noexcept;
Proceso construirProcesoInicial(const AltaProceso& alta);
bool cpuConsistente(const Proceso& proceso) noexcept;
bool procesoConsistente(const Proceso& proceso) noexcept;

// Recalcula en lote la prioridad de READY/RUN/WAIT. Los terminales se ignoran.
// Devuelve false para una población activa con tamaños no positivos; no muta
// prioridades en ese caso. Una población vacía es válida y no se modifica.
bool clasificarPrioridades(std::vector<Proceso>& procesos) noexcept;

// Calcula una prioridad para un tamaño en [minimo, maximo], con extremos
// positivos. Las fronteras son estrictas y la evaluación evita productos que
// pudieran desbordarse. Fuera de precondición devuelve P3.
Prioridad clasificarTamano(std::int64_t tamanioBytes,
                           std::int64_t minimoBytes,
                           std::int64_t maximoBytes) noexcept;

}  // namespace ConcurrentSimulator::v2
