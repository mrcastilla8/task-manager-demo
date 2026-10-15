#include "gui/Presentation.h"
#include "gui/PreviewFrames.h"
#include "tests/TestSupport.h"
#include <limits>

int main() {
    using namespace gui;
    api::EstadoSistema state;
    state.tiempoSimuladoMs = 27;
    state.timeline = {{1, 2, 8}, {2, 18, 22}};
    const auto spans = timelineSpans(state);
    EXPECT_EQ(spans.size(), 5U);
    EXPECT_EQ(spans[0].pid, -1);
    EXPECT_EQ(spans[0].start, 0);
    EXPECT_EQ(spans[0].end, 2);
    EXPECT_EQ(spans[1].pid, 1);
    EXPECT_EQ(spans[2].start, 8);
    EXPECT_EQ(spans[2].end, 18);
    EXPECT_EQ(spans[4].start, 22);
    EXPECT_EQ(spans[4].end, 27);
    EXPECT_EQ(state.timeline.size(), 2U); // Gaps jamás se insertan en el contrato.
    EXPECT_EQ(state.tiempoSimuladoMs, 27);
    state.timeline.emplace_back(3, 21, 23);
    EXPECT_THROW(timelineSpans(state), std::invalid_argument);
    state.timeline = {{1, 0, 28}};
    EXPECT_THROW(timelineSpans(state), std::invalid_argument);
    state.timeline.clear();
    EXPECT_EQ(timelineSpans(state).size(), 1U);
    state.tiempoSimuladoMs = 0;
    EXPECT_TRUE(timelineSpans(state).empty());
    state.tiempoSimuladoMs = -1;
    EXPECT_THROW(timelineSpans(state), std::invalid_argument);

    api::AltaProceso input{1, "Editor", std::numeric_limits<std::int64_t>::max(), 100, {true, 8, 10}};
    EXPECT_TRUE(validateInput(input).empty());
    input.operacionIO.despuesDeCpuMs = 100;
    EXPECT_FALSE(validateInput(input).empty());
    input.operacionIO.habilitada = false;
    EXPECT_TRUE(validateInput(input).empty());
    input.nombre = "  \t";
    EXPECT_FALSE(validateInput(input).empty());

    const auto frames = previewFrames();
    EXPECT_TRUE(frames.size() >= 7);
    bool run = false, wait = false, finish = false, cancel = false;
    for (const auto& frame : frames) {
        std::int64_t used = 0;
        int runners = 0;
        for (const auto& p : frame.state.procesos) {
            if (active(p.estado)) used += p.tamanioBytes;
            if (p.estado == api::Estado::RUN) { run = true; ++runners; }
            wait |= p.estado == api::Estado::WAIT;
            finish |= p.estado == api::Estado::FINISH;
            cancel |= p.estado == api::Estado::CANCEL;
            EXPECT_EQ(p.tiempoCpuTotalMs, p.tiempoCpuRestanteMs + p.tiempoCpuConsumidoMs);
        }
        EXPECT_TRUE(runners <= 1);
        EXPECT_EQ(used, frame.state.memoriaUsadaBytes);
        for (int pid : frame.state.colaReady) {
            const auto* p = findProcess(frame.state, pid);
            EXPECT_TRUE(p && p->estado == api::Estado::READY);
        }
        const auto rendered = timelineSpans(frame.state);
        if (!rendered.empty()) EXPECT_EQ(rendered.back().end, frame.state.tiempoSimuladoMs);
    }
    EXPECT_TRUE(run && wait && finish && cancel);
    return test::finish();
}
