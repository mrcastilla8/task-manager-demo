#pragma once
#include "contracts/v2/EstadoSistema.h"
#include <string>
#include <vector>
namespace gui {
struct PreviewFrame {
    std::string title;
    ConcurrentSimulator::v2::EstadoSistema state;
};
// Fixtures de presentación, NO implementa IExecutionEngine ni selecciona procesos.
struct PreviewScenario {
    std::string title;
    std::vector<PreviewFrame> frames;
};
std::vector<PreviewScenario> previewScenarios();

// Navegación de presentación; nunca cruza de escenario automáticamente.
class PreviewPlayback {
public:
    PreviewPlayback();
    const std::vector<PreviewScenario>& scenarios() const { return scenarios_; }
    const PreviewFrame& current() const;
    std::size_t scenarioIndex() const { return scenario_; }
    std::size_t frameIndex() const { return frame_; }
    std::size_t frameCount() const;
    bool atEnd() const;
    bool next();
    bool previous();
    void select(std::size_t index);
    void reset() { frame_ = 0; }
private:
    std::vector<PreviewScenario> scenarios_;
    std::size_t scenario_{0}, frame_{0};
};
}
