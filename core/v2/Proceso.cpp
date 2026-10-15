#include "core/v2/Proceso.h"

#include <algorithm>
#include <stdexcept>

namespace ConcurrentSimulator::v2 {
namespace {

bool activo(Estado estado) noexcept {
    return estado == Estado::READY || estado == Estado::RUN ||
           estado == Estado::WAIT;
}

}  // namespace

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
    switch (proceso.prioridad) {
        case Prioridad::P1:
        case Prioridad::P2:
        case Prioridad::P3:
            return true;
    }
    return false;
}

Prioridad clasificarTamano(std::int64_t tamanioBytes,
                           std::int64_t minimoBytes,
                           std::int64_t maximoBytes) noexcept {
    if (minimoBytes <= 0 || minimoBytes > maximoBytes ||
        tamanioBytes < minimoBytes || tamanioBytes > maximoBytes) {
        return Prioridad::P3;
    }
    const std::int64_t diferencia = maximoBytes - minimoBytes;
    if (diferencia == 0) {
        return Prioridad::P3;
    }

    const std::int64_t desplazamiento = tamanioBytes - minimoBytes;
    // 3*x < D equivale a x <= floor((D-1)/3).
    if (desplazamiento <= (diferencia - 1) / 3) {
        return Prioridad::P1;
    }

    // 3*x < 2*D equivale a x <= floor((2*D-1)/3). Escribiendo
    // D=3q+r, el umbral es 2q-1, 2q o 2q+1 para r=0,1,2. 2q no
    // desborda porque D <= INT64_MAX-1.
    const std::int64_t cociente = diferencia / 3;
    const std::int64_t residuo = diferencia % 3;
    const std::int64_t umbralP2 =
        2 * cociente + (residuo == 0 ? -1 : residuo - 1);
    if (desplazamiento <= umbralP2) {
        return Prioridad::P2;
    }
    return Prioridad::P3;
}

bool clasificarPrioridades(std::vector<Proceso>& procesos) noexcept {
    bool encontrado = false;
    std::int64_t minimo = 0;
    std::int64_t maximo = 0;
    for (const Proceso& proceso : procesos) {
        if (!activo(proceso.estado)) {
            continue;
        }
        if (proceso.tamanioBytes <= 0) {
            return false;
        }
        if (!encontrado) {
            minimo = proceso.tamanioBytes;
            maximo = proceso.tamanioBytes;
            encontrado = true;
        } else {
            minimo = (std::min)(minimo, proceso.tamanioBytes);
            maximo = (std::max)(maximo, proceso.tamanioBytes);
        }
    }
    if (!encontrado) {
        return true;
    }
    for (Proceso& proceso : procesos) {
        if (activo(proceso.estado)) {
            proceso.prioridad = clasificarTamano(
                proceso.tamanioBytes, minimo, maximo);
        }
    }
    return true;
}

}  // namespace ConcurrentSimulator::v2
