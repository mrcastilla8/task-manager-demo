#pragma once

#include "core/v2/ListaProcesos.h"
#include "core/v2/Memoria.h"
#include "core/v2/Proceso.h"

namespace ConcurrentSimulator::v2 {

// API provisional, local al módulo Castilla. No implementa transiciones,
// eventos, reloj ni scheduling y no forma parte de contracts/v2.
enum class ResultadoAdmision {
    ACEPTADO,
    ALTA_INVALIDA,
    PID_DUPLICADO,
    MEMORIA_INSUFICIENTE,
    INSERCION_RECHAZADA
};

ResultadoAdmision admitirProceso(const AltaProceso& alta,
                                 ListaProcesos& procesos,
                                 Memoria& memoria);

namespace detail {

// Punto de prueba interno de la transacción. La inserción puede agregar solo
// este PID; si rechaza o lanza, el helper quita ese PID y libera su reserva.
using InsertarProceso = bool (*)(ListaProcesos&, const Proceso&);
ResultadoAdmision admitirProcesoConInsercion(const AltaProceso& alta,
                                              ListaProcesos& procesos,
                                              Memoria& memoria,
                                              InsertarProceso insertar);

}  // namespace detail
}  // namespace ConcurrentSimulator::v2
