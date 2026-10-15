#include "gui/EngineSession.h"
#include <stdexcept>
namespace gui {
EngineSession::EngineSession(std::unique_ptr<Engine> engine) : engine_(std::move(engine)) {
    if (!engine_) throw std::invalid_argument("Se requiere un motor");
}
ConcurrentSimulator::v2::EstadoSistema EngineSession::snapshot() const {
    return engine_->obtenerEstado();
}
}
