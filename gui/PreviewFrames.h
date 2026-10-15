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
std::vector<PreviewFrame> previewFrames();
}
