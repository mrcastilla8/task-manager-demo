#include "gui/EngineSession.h"

#include "concurrent/ConcurrentEngineV2.h"
#include "simulation/SimulationEngineV2.h"

#include <stdexcept>
#include <utility>

namespace gui {
namespace api = ConcurrentSimulator::v2;

EngineSession::EngineSession(std::unique_ptr<Engine> engine)
    : engine_(std::move(engine)), hilo_(&EngineSession::loop, this) {
    if (!engine_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            detener_ = true;
        }
        cv_.notify_all();
        hilo_.join();
        throw std::invalid_argument("Se requiere un motor");
    }
}

EngineSession::~EngineSession() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        detener_ = true;
    }
    cv_.notify_all();
    if (hilo_.joinable()) {
        hilo_.join();
    }
}

void EngineSession::snapshot(SnapshotCallback callback) {
    encolar([this, callback = std::move(callback)]() mutable {
        try {
            callback(engine_->obtenerEstado(), nullptr);
        } catch (...) {
            callback({}, std::current_exception());
        }
    });
}

void EngineSession::prepararPaso(ResultCallback callback) {
    encolar([this, callback = std::move(callback)]() mutable {
        try {
            api::Resultado resultado = engine_->iniciar();
            if (resultado) {
                resultado = engine_->pausar();
            }
            callback(resultado, nullptr);
        } catch (...) {
            callback({}, std::current_exception());
        }
    });
}

void EngineSession::pausar(ResultCallback callback) {
    encolar([this, callback = std::move(callback)]() mutable {
        try {
            callback(engine_->pausar(), nullptr);
        } catch (...) {
            callback({}, std::current_exception());
        }
    });
}

void EngineSession::agregar(api::AltaProceso proceso, ResultCallback callback) {
    encolar([this, proceso = std::move(proceso),
             callback = std::move(callback)]() mutable {
        try {
            callback(engine_->agregarProceso(proceso), nullptr);
        } catch (...) {
            callback({}, std::current_exception());
        }
    });
}

void EngineSession::cancelar(int pid, ResultCallback callback) {
    encolar([this, pid, callback = std::move(callback)]() mutable {
        try {
            callback(engine_->cancelarProceso(pid), nullptr);
        } catch (...) {
            callback({}, std::current_exception());
        }
    });
}

void EngineSession::siguientePaso(StepCallback callback) {
    encolar([this, callback = std::move(callback)]() mutable {
        try {
            callback(engine_->siguientePaso(), nullptr);
        } catch (...) {
            callback({}, std::current_exception());
        }
    });
}

void EngineSession::reiniciarParaPasos(ResultCallback callback) {
    encolar([this, callback = std::move(callback)]() mutable {
        try {
            api::Resultado resultado = engine_->reiniciar();
            if (resultado) {
                resultado = engine_->iniciar();
            }
            if (resultado) {
                resultado = engine_->pausar();
            }
            callback(resultado, nullptr);
        } catch (...) {
            callback({}, std::current_exception());
        }
    });
}

std::unique_ptr<EngineSession::Engine> EngineSession::crearLogico(
    std::int64_t memoriaBytes) {
    return std::make_unique<api::SimulationEngineV2>(
        api::Configuracion{memoriaBytes});
}

std::unique_ptr<EngineSession::Engine> EngineSession::crearConcurrente(
    std::int64_t memoriaBytes) {
    return std::make_unique<api::ConcurrentEngineV2>(
        api::Configuracion{memoriaBytes});
}

void EngineSession::loop() {
    while (true) {
        std::function<void()> trabajo;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return detener_ || !trabajos_.empty(); });
            if (trabajos_.empty() && detener_) {
                return;
            }
            trabajo = std::move(trabajos_.front());
            trabajos_.pop();
        }
        trabajo();
    }
}

void EngineSession::encolar(std::function<void()> trabajo) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (detener_) {
            throw std::runtime_error("La sesión de motor está cerrándose");
        }
        trabajos_.push(std::move(trabajo));
    }
    cv_.notify_one();
}

}  // namespace gui
