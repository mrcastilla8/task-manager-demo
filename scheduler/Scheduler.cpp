#include "scheduler/Scheduler.h"

#include <algorithm>

Scheduler::Scheduler(SchedulerConfig config) noexcept
    : config_(config) {}

const SchedulerConfig& Scheduler::configuracion() const noexcept {
    return config_;
}

void Scheduler::configurar(const SchedulerConfig& config) noexcept {
    config_ = config;
}

int Scheduler::seleccionarSiguiente(
    const std::vector<Proceso>& procesos) const {
    const Proceso* mejor = nullptr;

    for (const auto& proceso : procesos) {
        if (proceso.estado != Estado::READY) {
            continue;
        }

        if (mejor == nullptr) {
            mejor = &proceso;
            continue;
        }

        if (proceso.prioridadActual > mejor->prioridadActual) {
            mejor = &proceso;
        } else if (proceso.prioridadActual == mejor->prioridadActual) {
            if (proceso.ordenLlegada < mejor->ordenLlegada) {
                mejor = &proceso;
            } else if (proceso.ordenLlegada == mejor->ordenLlegada &&
                       proceso.pid < mejor->pid) {
                mejor = &proceso;
            }
        }
    }

    return mejor != nullptr ? mejor->pid : -1;
}

void Scheduler::aplicarAging(std::vector<Proceso>& procesos,
                             std::int64_t deltaTiempo) const {
    if (deltaTiempo <= 0 || config_.agingIntervaloMs <= 0) {
        return;
    }

    for (auto& proceso : procesos) {
        if (proceso.estado != Estado::READY) {
            continue;
        }

        const std::int64_t esperaPrevia = proceso.tiempoEspera;
        proceso.tiempoEspera += deltaTiempo;

        const std::int64_t intervalosPrevios =
            esperaPrevia / config_.agingIntervaloMs;
        const std::int64_t intervalosActuales =
            proceso.tiempoEspera / config_.agingIntervaloMs;
        const std::int64_t incrementoTotal =
            (intervalosActuales - intervalosPrevios) * config_.agingIncremento;

        if (incrementoTotal > 0) {
            const auto nuevaPrioridad = static_cast<std::int64_t>(
                proceso.prioridadActual) + incrementoTotal;
            proceso.prioridadActual = static_cast<int>(
                std::min<std::int64_t>(config_.prioridadMaxima, nuevaPrioridad));
        }
    }
}

void Scheduler::alAsignarCPU(Proceso& proceso) const noexcept {
    proceso.prioridadActual = proceso.prioridadBase;
    proceso.tiempoEspera = 0;
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
              [](const Proceso* a, const Proceso* b) {
                  if (a->prioridadActual != b->prioridadActual) {
                      return a->prioridadActual > b->prioridadActual;
                  }
                  if (a->ordenLlegada != b->ordenLlegada) {
                      return a->ordenLlegada < b->ordenLlegada;
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
