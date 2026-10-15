#pragma once

#include "contracts/v2/IExecutionEngine.h"
#include "concurrent/ProcessThread.h"
#include "simulation/SimulationEngineV2.h"

#include <condition_variable>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

namespace ConcurrentSimulator::v2 {

// El coordinador serializa el acceso a SimulationEngineV2. Cada proceso READY
// tiene un worker real que calcula su ráfaga; el motor lógico sigue siendo la
// autoridad de las transiciones, reloj, memoria, eventos y timeline.
class ConcurrentEngineV2 final : public IExecutionEngine {
public:
    explicit ConcurrentEngineV2(Configuracion configuracion);
    ~ConcurrentEngineV2() override;

    ConcurrentEngineV2(const ConcurrentEngineV2&) = delete;
    ConcurrentEngineV2& operator=(const ConcurrentEngineV2&) = delete;

    Resultado agregarProceso(const AltaProceso& proceso) override;
    Resultado cancelarProceso(int pid) override;
    Resultado iniciar() override;
    Resultado pausar() override;
    Resultado reiniciar() override;
    ResultadoPaso siguientePaso() override;
    EstadoSistema obtenerEstado() const override;

    std::size_t cantidadWorkers() const;

private:
    template <typename Funcion>
    auto encolar(Funcion&& funcion) const -> decltype(funcion()) {
        using Retorno = decltype(funcion());
        auto tarea = std::make_shared<std::packaged_task<Retorno()>>(
            std::forward<Funcion>(funcion));
        std::future<Retorno> resultado = tarea->get_future();
        {
            std::lock_guard<std::mutex> lock(mutexCola_);
            if (detenerCoordinador_) {
                throw std::runtime_error("El coordinador ya está detenido");
            }
            trabajos_.emplace([tarea]() { (*tarea)(); });
        }
        cvTrabajos_.notify_one();
        return resultado.get();
    }

    void loopCoordinador();
    ResultadoPaso siguientePasoInterno();
    bool IOConsumida(int pid, const EstadoSistema& estado) const;
    void detenerWorkers();

    SimulationEngineV2 motor_;
    std::map<int, std::unique_ptr<::ProcessThread>> workers_;
    mutable std::mutex mutexCola_;
    mutable std::condition_variable cvTrabajos_;
    mutable std::queue<std::function<void()>> trabajos_;
    mutable bool detenerCoordinador_{false};
    std::thread coordinador_;
};

}  // namespace ConcurrentSimulator::v2
