#pragma once
#include "gui/EngineSession.h"
#include "gui/PreviewFrames.h"
#include "gui/Presentation.h"
#include "gui/SnapshotComparison.h"
#include <QMainWindow>
#include <functional>
#include <memory>
#include <optional>
class QLabel;
class QTimer;
class QComboBox;
class QPushButton;
namespace gui {
class ProcessTable;
class QueueWidget;
class CPUWidget;
class MemoryWidget;
class TimelineWidget;
class EventLogWidget;
class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    // Punto de composición experimental, no implica adopción de la API candidata.
    // El motor debe ofrecer llamadas no bloqueantes desde UI antes de conectarse.
    void attachEngine(std::unique_ptr<EngineSession::Engine> engine, const QString& label);
private:
    void present(const api::EstadoSistema& state);
    void showFrame();
    void advancePreview();
    void refreshEngine();
    void action(const std::function<void()>& operation);
    void report(api::CodigoResultado result);
    void addProcess();
    void updateControls(const api::EstadoSistema& state);
    void updateCancellation();
    void clearVisualContext();
    std::unique_ptr<EngineSession> session_;
    PreviewPlayback playback_;
    std::optional<api::EstadoSistema> lastSnapshot_;
    SnapshotRetry retry_;
    bool snapshotHealthy_{false};
    QLabel *banner_, *clock_, *status_;
    QTimer *previewTimer_, *refreshTimer_;
    QComboBox *sources_, *scenarios_;
    QPushButton *start_, *pause_, *step_, *reset_, *add_, *cancel_, *previous_, *retryButton_;
    ProcessTable* processes_;
    QueueWidget* queue_;
    CPUWidget* cpu_;
    MemoryWidget* memory_;
    TimelineWidget* timeline_;
    EventLogWidget* events_;
};
}
