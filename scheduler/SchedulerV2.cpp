#include "scheduler/SchedulerV2.h"

#include <algorithm>
#include <climits>
#include <stdexcept>

namespace ConcurrentSimulator::v2 {

namespace {

inline bool perteneceATL(Estado estado) noexcept {
    return estado == Estado::READY || estado == Estado::RUN || estado == Estado::WAIT;
}

}  // namespace

Prioridad Scheduler::evaluarPrioridad(std::int64_t tamanioBytes,
                                     std::int64_t minA,
                                     std::int64_t maxB) {
    if (tamanioBytes < 0 || minA < 0 || maxB < 0) {
        throw std::invalid_argument("Scheduler::evaluarPrioridad: los tamanos no pueden ser negativos");
    }
    if (minA > maxB) {
        throw std::invalid_argument("Scheduler::evaluarPrioridad: minA no puede ser mayor que maxB");
    }
    if (tamanioBytes < minA || tamanioBytes > maxB) {
        throw std::invalid_argument("Scheduler::evaluarPrioridad: tamanioBytes fuera del intervalo [minA, maxB]");
    }

    if (minA == maxB) {
        return Prioridad::P3;
    }

    const std::int64_t D = maxB - minA;
    if (D <= 0) {
        return Prioridad::P3;
    }

    const std::int64_t diff = tamanioBytes - minA;

    // Evaluacion exacta y libre de overflow de: 3 * diff < D
    constexpr std::int64_t limiteTercio = INT64_MAX / 3;
    bool esP1 = false;
    if (diff <= limiteTercio) {
        esP1 = (3 * diff < D);
    } else {
        // Si diff > INT64_MAX / 3, entonces 3 * diff > INT64_MAX >= D, por tanto no puede ser menor que D
        esP1 = false;
    }

    if (esP1) {
        return Prioridad::P1;
    }

    // Evaluacion exacta y libre de overflow de: 3 * diff < 2 * D
    // Equivalencia matematica: 3 * (maxB - tamanioBytes) > D
    const std::int64_t diffB = maxB - tamanioBytes;
    bool esP2 = false;
    if (diffB <= 0) {
        esP2 = false;
    } else if (diffB > limiteTercio) {
        // Si diffB > INT64_MAX / 3, entonces 3 * diffB > INT64_MAX >= D, lo cual es siempre verdadero
        esP2 = true;
    } else {
        esP2 = (3 * diffB > D);
    }

    if (esP2) {
        return Prioridad::P2;
    }

    return Prioridad::P3;
}

void Scheduler::clasificarPrioridades(std::vector<Proceso>& procesos) const {
    std::int64_t minA = INT64_MAX;
    std::int64_t maxB = INT64_MIN;
    bool hayTL = false;

    for (const auto& proc : procesos) {
        if (perteneceATL(proc.estado)) {
            if (proc.tamanioBytes < 0) {
                throw std::invalid_argument("Scheduler::clasificarPrioridades: proceso en TL con tamanio negativo");
            }
            minA = std::min(minA, proc.tamanioBytes);
            maxB = std::max(maxB, proc.tamanioBytes);
            hayTL = true;
        }
    }

    if (!hayTL) {
        return;
    }

    for (auto& proc : procesos) {
        if (perteneceATL(proc.estado)) {
            proc.prioridad = evaluarPrioridad(proc.tamanioBytes, minA, maxB);
        }
    }
}

int Scheduler::seleccionarSiguiente(const std::vector<Proceso>& procesos) const noexcept {
    const Proceso* mejor = nullptr;

    for (const auto& proceso : procesos) {
        if (proceso.estado != Estado::READY) {
            continue;
        }

        if (mejor == nullptr) {
            mejor = &proceso;
            continue;
        }

        const auto prioActual = static_cast<std::uint8_t>(proceso.prioridad);
        const auto prioMejor = static_cast<std::uint8_t>(mejor->prioridad);

        if (prioActual > prioMejor) {
            mejor = &proceso;
        } else if (prioActual == prioMejor) {
            if (proceso.ticketReady < mejor->ticketReady) {
                mejor = &proceso;
            } else if (proceso.ticketReady == mejor->ticketReady &&
                       proceso.pid < mejor->pid) {
                mejor = &proceso;
            }
        }
    }

    return mejor != nullptr ? mejor->pid : -1;
}

std::vector<int> Scheduler::ordenarColaPrioridad(
    const std::vector<Proceso>& procesos) const {
    std::vector<const Proceso*> listos;
    listos.reserve(procesos.size());

    for (const auto& proceso : procesos) {
        if (proceso.estado == Estado::READY) {
            listos.push_back(&proceso);
        }
    }

    std::sort(listos.begin(), listos.end(),
              [](const Proceso* a, const Proceso* b) noexcept {
                  const auto prioA = static_cast<std::uint8_t>(a->prioridad);
                  const auto prioB = static_cast<std::uint8_t>(b->prioridad);

                  if (prioA != prioB) {
                      return prioA > prioB;
                  }
                  if (a->ticketReady != b->ticketReady) {
                      return a->ticketReady < b->ticketReady;
                  }
                  return a->pid < b->pid;
              });

    std::vector<int> resultado;
    resultado.reserve(listos.size());
    for (const auto* proceso : listos) {
        resultado.push_back(proceso->pid);
    }

    return resultado;
}

}  // namespace ConcurrentSimulator::v2
