#include "gui/Presentation.h"
#include "gui/PreviewFrames.h"
#include "gui/SnapshotComparison.h"
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

    const auto scenarios = previewScenarios();
    EXPECT_EQ(scenarios.size(), 3U);
    EXPECT_EQ(scenarios.front().frames.size(), 7U);
    bool run = false, wait = false, finish = false, cancel = false;
    for (const auto& scenario : scenarios) {
      std::int64_t previousTime = 0;
      for (const auto& frame : scenario.frames) {
        EXPECT_TRUE(frame.state.tiempoSimuladoMs >= previousTime);
        previousTime = frame.state.tiempoSimuladoMs;
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
    }
    EXPECT_TRUE(run && wait && finish && cancel);

    PreviewPlayback playback;
    while (playback.next()) {}
    EXPECT_EQ(playback.current().state.tiempoSimuladoMs, 30);
    EXPECT_EQ(playback.scenarioIndex(), 0U);
    EXPECT_FALSE(playback.next());
    EXPECT_EQ(playback.frameIndex(), 6U);
    playback.select(2);
    EXPECT_EQ(playback.frameIndex(), 0U);
    EXPECT_EQ(playback.current().state.tiempoSimuladoMs, 27);
    EXPECT_FALSE(playback.next());
    EXPECT_FALSE(playback.previous());
    playback.select(0);
    EXPECT_EQ(playback.current().state.tiempoSimuladoMs, 0);
    playback.next();
    playback.reset();
    EXPECT_EQ(playback.frameIndex(), 0U);
    EXPECT_THROW(playback.select(99), std::out_of_range);

    SnapshotRetry retry;
    EXPECT_EQ(retry.delayMs(), 150);
    retry.failure();
    EXPECT_FALSE(retry.healthy());
    EXPECT_EQ(retry.delayMs(), 300);
    for (int i = 0; i < 100; ++i) retry.failure();
    EXPECT_EQ(retry.delayMs(), 5000);
    retry.success();
    EXPECT_TRUE(retry.healthy());
    EXPECT_EQ(retry.delayMs(), 150);

    auto original = scenarios.front().frames[3].state;
    auto copy = original;
    EXPECT_TRUE(sameSnapshot(original, copy));
    copy.eventos.front().detalle += "corregido";
    EXPECT_FALSE(sameSnapshot(original, copy));
    copy = original;
    copy.procesos.front().operacionIO.duracionEsperaMs += 1;
    EXPECT_FALSE(sameSnapshot(original, copy));
    copy = original;
    copy.timeline.back() = api::BloqueTimeline(1, 4, 6);
    EXPECT_FALSE(sameSnapshot(original, copy));
    copy = original;
    copy.colaReady.clear();
    EXPECT_FALSE(sameSnapshot(original, copy));
    return test::finish();
}
