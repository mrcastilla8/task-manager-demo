#include "core/ColaProcesosV2.h"
#include "scheduler/SchedulerV2.h"

#include <algorithm>
#include <stdexcept>

namespace ConcurrentSimulator::v2 {

namespace {

inline bool procesoValido(const Proceso& proceso) noexcept {
    const bool camposBasicosValidos =
        proceso.pid > 0 &&
        !proceso.nombre.empty() &&
        proceso.tamanioBytes > 0 &&
        proceso.tiempoCpuTotalMs > 0 &&
        proceso.tiempoCpuRestanteMs >= 0 &&
        proceso.tiempoCpuRestanteMs <= proceso.tiempoCpuTotalMs &&
        proceso.tiempoCpuConsumidoMs >= 0 &&
        proceso.tiempoEsperaMs >= 0;

    if (!camposBasicosValidos || !proceso.operacionIO.habilitada) {
        return camposBasicosValidos;
    }

    const OperacionIO& op = proceso.operacionIO;
    return op.despuesDeCpuMs > 0 &&
           op.despuesDeCpuMs < proceso.tiempoCpuTotalMs &&
           op.duracionEsperaMs > 0;
}

}  // namespace

void ColaProcesos::agregar(const Proceso& proceso) {
    if (!procesoValido(proceso)) {
        throw std::invalid_argument("ColaProcesos::agregar: el proceso contiene campos invalidos (tamano <= 0, PID <= 0, etc.)");
    }
    if (buscar(proceso.pid) != nullptr) {
        throw std::invalid_argument("ColaProcesos::agregar: ya existe un proceso con el mismo PID");
    }

    procesos_.push_back(proceso);
}

bool ColaProcesos::eliminar(int pid) {
    const auto it = std::find_if(
        procesos_.begin(), procesos_.end(),
        [pid](const Proceso& actual) noexcept { return actual.pid == pid; });

    if (it == procesos_.end()) {
        return false;
    }

    procesos_.erase(it);
    return true;
}

Proceso* ColaProcesos::buscar(int pid) noexcept {
    const auto it = std::find_if(
        procesos_.begin(), procesos_.end(),
        [pid](const Proceso& actual) noexcept { return actual.pid == pid; });
    return it == procesos_.end() ? nullptr : &*it;
}

const Proceso* ColaProcesos::buscar(int pid) const noexcept {
    const auto it = std::find_if(
        procesos_.cbegin(), procesos_.cend(),
        [pid](const Proceso& actual) noexcept { return actual.pid == pid; });
    return it == procesos_.cend() ? nullptr : &*it;
}

std::vector<Proceso> ColaProcesos::obtenerTodos() const {
    return {procesos_.cbegin(), procesos_.cend()};
}

std::vector<Proceso> ColaProcesos::obtenerPorEstado(Estado estado) const {
    std::vector<Proceso> resultado;
    for (const auto& proceso : procesos_) {
        if (proceso.estado == estado) {
            resultado.push_back(proceso);
        }
    }
    return resultado;
}

void ColaProcesos::recalcularPrioridades(const Scheduler& scheduler) {
    // La responsabilidad de calcular prioridades es exclusivamente del Scheduler.
    // La cola solo persiste las prioridades calculadas in-place en su lista interna.
    scheduler.clasificarPrioridades(procesos_);
}

}  // namespace ConcurrentSimulator::v2
