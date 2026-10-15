#pragma once

#include "contracts/v2/IExecutionEngine.h"
#include <string>
#include <vector>

namespace gui {
namespace api = ConcurrentSimulator::v2;

const char* stateName(api::Estado value);
const char* executionName(api::EstadoEjecucion value);
const char* eventName(api::TipoEvento value);
const char* resultName(api::CodigoResultado value);
bool active(api::Estado value);
const api::Proceso* findProcess(const api::EstadoSistema& state, int pid);

// Intervalos de PRESENTACIÓN; pid=-1 significa hueco, nunca BloqueTimeline.
struct TimelineSpan {
    int pid;
    std::int64_t start;
    std::int64_t end;
};
std::vector<TimelineSpan> timelineSpans(const api::EstadoSistema& state);
std::string validateInput(const api::AltaProceso& input);
} // namespace gui
