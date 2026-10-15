#pragma once
#include "contracts/v2/IExecutionEngine.h"
#include <memory>

namespace gui {
// Único adaptador a API CANDIDATA. No tiene dependencias Qt ni driver continuo.
// Solo conectar motores cuando el equipo ratifique API, latencia y ciclo de vida.
class EngineSession {
public:
    using Engine = ConcurrentSimulator::v2::IExecutionEngine;
    explicit EngineSession(std::unique_ptr<Engine> engine);
    Engine& engine() { return *engine_; }
    ConcurrentSimulator::v2::EstadoSistema snapshot() const;
private:
    std::unique_ptr<Engine> engine_;
};
}
