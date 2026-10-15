#include "gui/ProcessTable.h"
#include "gui/EventLogWidget.h"
#include "gui/TimelineWidget.h"
#include "gui/CPUWidget.h"
#include "gui/PreviewFrames.h"
#include "tests/TestSupport.h"
#include <QApplication>
#include <QGraphicsScene>
#include <QLabel>
#include <QScrollBar>
#include <QSignalSpy>
#include <algorithm>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    using namespace gui;
    api::EstadoSistema state;
    constexpr int entries = 50000;
    state.eventos.reserve(entries + 1);
    state.timeline.reserve(entries + 1);
    state.procesos.reserve(5000);
    for (int i = 0; i < entries; ++i) {
        state.eventos.push_back({i * 2, api::TipoEvento::CPU_EJECUTADA, 1, 1, "Historial extenso"});
        state.timeline.emplace_back(1, i * 2, i * 2 + 1);
    }
    state.tiempoSimuladoMs = entries * 2;
    for (int i = 0; i < 5000; ++i)
        state.procesos.push_back({i + 1, "Proceso", 64, api::Prioridad::P3, 10, 10, 0, 0,
            static_cast<std::uint64_t>(i + 1), api::Estado::READY, {}});
    ProcessTable processes;
    EventLogWidget events;
    TimelineWidget timeline;
    processes.resize(700, 200);
    events.resize(700, 200);
    timeline.resize(700, 200);
    QSignalSpy inserted(events.model(), &QAbstractItemModel::rowsInserted);
    QSignalSpy changed(events.model(), &QAbstractItemModel::dataChanged);
    QSignalSpy reset(events.model(), &QAbstractItemModel::modelReset);
    QSignalSpy processChanged(processes.model(), &QAbstractItemModel::dataChanged);
    processes.present(state);
    events.present(state);
    timeline.present(state);
    EXPECT_EQ(inserted.count(), 1);
    EXPECT_EQ(events.model()->rowCount(), entries);
    EXPECT_EQ(processes.model()->rowCount(), 5000);
    EXPECT_EQ(timeline.scene()->items().size(), 1); // No un item por intervalo.
    EXPECT_EQ(timeline.spanCount(), static_cast<std::size_t>(entries * 2));
    processes.selectRow(100);
    processes.verticalScrollBar()->setValue(80);
    processes.horizontalScrollBar()->setValue(140);
    events.verticalScrollBar()->setValue(100);
    events.horizontalScrollBar()->setValue(160);
    timeline.setZoom(30);
    timeline.horizontalScrollBar()->setValue(10000);
    EXPECT_TRUE(processes.verticalScrollBar()->maximum() > 80);
    EXPECT_TRUE(events.verticalScrollBar()->maximum() > 100);
    EXPECT_TRUE(timeline.horizontalScrollBar()->maximum() > 10000);
    const int px = processes.horizontalScrollBar()->value(), py = processes.verticalScrollBar()->value();
    const int ex = events.horizontalScrollBar()->value(), ey = events.verticalScrollBar()->value();
    const int tx = timeline.horizontalScrollBar()->value();
    const auto eventRevision = events.presentationModel()->revision();
    const auto processRevision = processes.presentationModel()->revision();
    const auto timelineRevision = timeline.revision();
    for (int i = 0; i < 20; ++i) {
        processes.present(state);
        events.present(state);
        timeline.present(state);
    }
    EXPECT_EQ(events.presentationModel()->revision(), eventRevision);
    EXPECT_EQ(processes.presentationModel()->revision(), processRevision);
    EXPECT_EQ(timeline.revision(), timelineRevision);
    EXPECT_EQ(changed.count(), 0);
    EXPECT_EQ(inserted.count(), 1);
    EXPECT_EQ(reset.count(), 0);
    EXPECT_EQ(processChanged.count(), 0);
    EXPECT_EQ(processes.selectedPid(), 101);
    EXPECT_EQ(processes.verticalScrollBar()->value(), py);
    EXPECT_EQ(processes.horizontalScrollBar()->value(), px);
    EXPECT_EQ(events.verticalScrollBar()->value(), ey);
    EXPECT_EQ(events.horizontalScrollBar()->value(), ex);
    EXPECT_EQ(timeline.horizontalScrollBar()->value(), tx);
    EXPECT_EQ(timeline.zoom(), 30);

    state.eventos.push_back({entries * 2, api::TipoEvento::CPU_EJECUTADA, 2, 1, "Nuevo"});
    state.timeline.emplace_back(2, entries * 2, entries * 2 + 1);
    state.tiempoSimuladoMs += 2;
    state.procesos[100].tiempoEsperaMs = 33;
    events.present(state);
    processes.present(state);
    timeline.present(state);
    EXPECT_EQ(inserted.count(), 2);
    EXPECT_EQ(changed.count(), 0);
    EXPECT_EQ(reset.count(), 0);
    EXPECT_EQ(processChanged.count(), 1);
    EXPECT_EQ(processes.selectedPid(), 101);
    EXPECT_EQ(processes.verticalScrollBar()->value(), py);
    EXPECT_EQ(processes.horizontalScrollBar()->value(), px);
    EXPECT_EQ(events.verticalScrollBar()->value(), ey);
    EXPECT_EQ(events.horizontalScrollBar()->value(), ex);
    EXPECT_EQ(timeline.horizontalScrollBar()->value(), tx);
    EXPECT_EQ(timeline.zoom(), 30);
    EXPECT_EQ(timeline.scene()->items().size(), 1);

    // Edición interior con el mismo tamaño: no confiar solo en count/último evento.
    state.eventos[17].detalle = "Corrección interior";
    events.present(state);
    EXPECT_EQ(changed.count(), 1);
    EXPECT_EQ(events.model()->data(events.model()->index(17, 4)).toString(), QString("Corrección interior"));
    events.scrollToBottom();
    state.eventos.push_back({entries * 2 + 2, api::TipoEvento::SISTEMA_FINALIZADO, -1, 0, "Fin"});
    events.present(state);
    EXPECT_EQ(events.verticalScrollBar()->value(), events.verticalScrollBar()->maximum());

    std::swap(state.procesos[100], state.procesos[101]);
    processes.present(state);
    EXPECT_EQ(processes.selectedPid(), 101); // Identidad por PID, no índice de fila.
    EXPECT_EQ(processes.verticalScrollBar()->value(), py);
    state.procesos.erase(state.procesos.begin() + 101);
    processes.present(state);
    EXPECT_EQ(processes.selectedPid(), -1);
    processes.selectRow(0);
    processes.clearSelection();
    EXPECT_EQ(processes.selectedPid(), -1);

    auto run = previewScenarios()[0].frames[1].state;
    run.procesos[1].prioridad = api::Prioridad::P1;
    CPUWidget cpu;
    cpu.present(run);
    EXPECT_TRUE(cpu.findChild<QLabel*>("cpuDescription")->text().contains("cuota fijada: 30 ms"));
    processes.present(run);
    EXPECT_EQ(processes.model()->data(processes.model()->index(1, 5)).toString(), QString("10"));
    state.eventos.clear();
    events.present(state);
    EXPECT_EQ(events.model()->rowCount(), 0);
    EXPECT_EQ(reset.count(), 0);
    EXPECT_EQ(state.tiempoSimuladoMs, entries * 2 + 2); // Ninguna vista escribió el reloj.
    return test::finish();
}
