#include "core/v2/Proceso.h"

#include <stdexcept>

namespace ConcurrentSimulator::v2 {

bool validarAltaProceso(const AltaProceso& alta) noexcept {
    if (alta.pid <= 0 || alta.nombre.empty() || alta.tamanioBytes <= 0 ||
        alta.tiempoCpuTotalMs <= 0) {
        return false;
    }
    if (!alta.operacionIO.habilitada) {
        return true;
    }
    return alta.operacionIO.despuesDeCpuMs > 0 &&
           alta.operacionIO.despuesDeCpuMs < alta.tiempoCpuTotalMs &&
           alta.operacionIO.duracionEsperaMs > 0;
}

Proceso construirProcesoInicial(const AltaProceso& alta) {
    if (!validarAltaProceso(alta)) {
        throw std::invalid_argument("AltaProceso contiene datos invalidos");
    }

    Proceso proceso;
    proceso.pid = alta.pid;
    proceso.nombre = alta.nombre;
    proceso.tamanioBytes = alta.tamanioBytes;
    proceso.prioridad = Prioridad::P3;
    proceso.tiempoCpuTotalMs = alta.tiempoCpuTotalMs;
    proceso.tiempoCpuRestanteMs = alta.tiempoCpuTotalMs;
    proceso.tiempoCpuConsumidoMs = 0;
    proceso.estado = Estado::READY;
    proceso.operacionIO = alta.operacionIO;
    return proceso;
}

bool cpuConsistente(const Proceso& proceso) noexcept {
    return proceso.tiempoCpuTotalMs > 0 &&
           proceso.tiempoCpuConsumidoMs >= 0 &&
           proceso.tiempoCpuRestanteMs >= 0 &&
           proceso.tiempoCpuConsumidoMs <= proceso.tiempoCpuTotalMs &&
           proceso.tiempoCpuRestanteMs ==
               proceso.tiempoCpuTotalMs - proceso.tiempoCpuConsumidoMs;
}

bool procesoConsistente(const Proceso& proceso) noexcept {
    if (proceso.pid <= 0 || proceso.nombre.empty() ||
        proceso.tamanioBytes <= 0 || proceso.tiempoCpuTotalMs <= 0 ||
        !cpuConsistente(proceso) || proceso.tiempoEsperaMs < 0) {
        return false;
    }
    if (proceso.operacionIO.habilitada &&
        (proceso.operacionIO.despuesDeCpuMs <= 0 ||
         proceso.operacionIO.despuesDeCpuMs >= proceso.tiempoCpuTotalMs ||
         proceso.operacionIO.duracionEsperaMs <= 0)) {
        return false;
    }
    switch (proceso.estado) {
        case Estado::READY:
        case Estado::RUN:
        case Estado::WAIT:
        case Estado::FINISH:
        case Estado::CANCEL:
            break;
        default:
            return false;
    }
    if (proceso.estado == Estado::FINISH &&
        proceso.tiempoCpuRestanteMs != 0) {
        return false;
    }
    switch (proceso.prioridad) {
        case Prioridad::P1:
        case Prioridad::P2:
        case Prioridad::P3:
            return true;
    }
    return false;
}

}  // namespace ConcurrentSimulator::v2
