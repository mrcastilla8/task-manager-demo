#pragma once

#include "contracts/v2/IExecutionEngine.h"

#include <condition_variable>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace gui {

// Serializa cada sesión en un hilo de transporte. La GUI solo recibe resultados
// por callbacks y nunca espera una operación del motor en el hilo Qt.
class EngineSession {
public:
    using Engine = ConcurrentSimulator::v2::IExecutionEngine;
    using Error = std::exception_ptr;
    using SnapshotCallback =
        std::function<void(ConcurrentSimulator::v2::EstadoSistema, Error)>;
    using ResultCallback =
        std::function<void(ConcurrentSimulator::v2::Resultado, Error)>;
    using StepCallback =
        std::function<void(ConcurrentSimulator::v2::ResultadoPaso, Error)>;

    explicit EngineSession(std::unique_ptr<Engine> engine);
    ~EngineSession();
    EngineSession(const EngineSession&) = delete;
    EngineSession& operator=(const EngineSession&) = delete;

    void snapshot(SnapshotCallback callback);
    void prepararPaso(ResultCallback callback);
    void pausar(ResultCallback callback);
    void agregar(ConcurrentSimulator::v2::AltaProceso proceso,
                 ResultCallback callback);
    void cancelar(int pid, ResultCallback callback);
    void siguientePaso(StepCallback callback);
    void reiniciarParaPasos(ResultCallback callback);

    static std::unique_ptr<Engine> crearLogico(std::int64_t memoriaBytes);
    static std::unique_ptr<Engine> crearConcurrente(std::int64_t memoriaBytes);

private:
    void loop();
    void encolar(std::function<void()> trabajo);

    std::unique_ptr<Engine> engine_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> trabajos_;
    bool detener_{false};
    std::thread hilo_;
};

}  // namespace gui
