#include "gui/MainWindow.h"
#include "gui/NewProcessDialog.h"
#include "gui/ProcessTable.h"
#include "gui/PreviewFrames.h"
#include "gui/TimelineWidget.h"
#include "gui/MemoryWidget.h"
#include "gui/EventLogWidget.h"
#include "tests/TestSupport.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <atomic>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace {
// Doble de transporte: no ejecuta procesos ni modifica el reloj.
class RecordingEngine : public gui::EngineSession::Engine {
public:
    gui::api::EstadoSistema state = gui::previewScenarios().front().frames.front().state;
    mutable std::atomic<int> reads{0};
    std::atomic<int> steps{0}, adds{0}, resets{0}, cancellations{0};
    std::atomic<bool> fail{false};
    mutable std::atomic<bool> readOffUiThread{false};
    mutable std::mutex stateMutex;
    gui::api::Resultado agregarProceso(const gui::api::AltaProceso&) override { ++adds; return {}; }
    gui::api::Resultado cancelarProceso(int pid) override { ++cancellations; return {gui::api::CodigoResultado::OK, pid}; }
    gui::api::Resultado iniciar() override { return {}; }
    gui::api::Resultado pausar() override { return {}; }
    gui::api::Resultado reiniciar() override { ++resets; return {}; }
    gui::api::ResultadoPaso siguientePaso() override { ++steps; return {}; }
    gui::api::EstadoSistema obtenerEstado() const override {
        readOffUiThread = QThread::currentThread() != qApp->thread();
        ++reads;
        if (fail) throw std::runtime_error("Fallo de lectura controlado");
        std::lock_guard<std::mutex> lock(stateMutex);
        return state;
    }
    gui::api::EstadoSistema stateCopy() const {
        std::lock_guard<std::mutex> lock(stateMutex);
        return state;
    }
    void setFirstState(gui::api::Estado value) {
        std::lock_guard<std::mutex> lock(stateMutex);
        state.procesos[0].estado = value;
    }
    void setInvalidTimeline(bool invalid) {
        std::lock_guard<std::mutex> lock(stateMutex);
        state.timeline.clear();
        if (invalid) state.timeline.emplace_back(1, 0, 1);
    }
};
QPushButton* button(gui::MainWindow& window, const char* name) { return window.findChild<QPushButton*>(name); }
template <typename T>
T* findWidget(QObject& root) {
    for (QWidget* candidate : root.findChildren<QWidget*>()) {
        if (auto* match = dynamic_cast<T*>(candidate)) return match;
    }
    return nullptr;
}
bool pumpUntil(const std::function<bool()>& condition) {
    QElapsedTimer elapsed;
    elapsed.start();
    while (!condition() && elapsed.elapsed() < 2000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        std::this_thread::yield();
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    return condition();
}
void refreshAndWait(gui::MainWindow& window, RecordingEngine& recording) {
    const int previous = recording.reads;
    auto* timer = window.findChild<QTimer*>("snapshotRefresh");
    button(window, "retrySnapshot")->click();
    EXPECT_TRUE(pumpUntil([&] {
        return recording.reads > previous && timer->isActive();
    }));
}
void checkPreview() {
    gui::MainWindow window; // Sin mostrar ventana ni generar capturas.
    auto* next = button(window, "step");
    auto* start = button(window, "start");
    auto* scenarios = window.findChild<QComboBox*>("previewScenario");
    auto* clock = window.findChild<QLabel*>("simulationClock");
    auto* table = dynamic_cast<gui::ProcessTable*>(window.findChild<QTableView*>("processTable"));
    EXPECT_EQ(scenarios->count(), 3);
    for (int i = 0; i < 6; ++i) next->click();
    EXPECT_TRUE(clock->text().contains("30 ms"));
    EXPECT_FALSE(next->isEnabled());
    EXPECT_FALSE(start->isEnabled());
    next->click();
    EXPECT_EQ(scenarios->currentIndex(), 0);
    EXPECT_TRUE(clock->text().contains("30 ms"));
    scenarios->setCurrentIndex(2);
    EXPECT_TRUE(clock->text().contains("27 ms"));
    EXPECT_FALSE(next->isEnabled());
    EXPECT_EQ(table->presentationModel()->rowCount(), 0);
    scenarios->setCurrentIndex(0);
    EXPECT_TRUE(clock->text().contains("0 ms"));
    EXPECT_EQ(table->presentationModel()->rowCount(), 3);
    next->click();
    EXPECT_TRUE(window.findChild<QLabel*>("cpuDescription")->text().contains("PID 2"));
    EXPECT_TRUE(clock->text().contains("0 ms"));
    button(window, "reset")->click();
    EXPECT_EQ(scenarios->currentIndex(), 0);
    EXPECT_TRUE(clock->text().contains("0 ms"));
}
void checkForm() {
    gui::NewProcessDialog dialog;
    dialog.accept();
    EXPECT_EQ(dialog.result(), 0);
    dialog.findChild<QLineEdit*>("inputName")->setText("Editor");
    dialog.findChild<QLineEdit*>("inputBytes")->setText("9223372036854775808");
    dialog.accept();
    EXPECT_EQ(dialog.result(), 0);
    dialog.findChild<QLineEdit*>("inputBytes")->setText("9223372036854775807");
    dialog.findChild<QCheckBox*>("inputIo")->setChecked(true);
    dialog.findChild<QLineEdit*>("inputIoAfter")->setText("20");
    dialog.accept();
    EXPECT_EQ(dialog.result(), 0);
    dialog.findChild<QLineEdit*>("inputIoAfter")->setText("8");
    dialog.accept();
    EXPECT_EQ(dialog.result(), static_cast<int>(QDialog::Accepted));
}
void checkFactories() {
    gui::MainWindow window;
    auto* sources = window.findChild<QComboBox*>("engineSource");
    auto* banner = window.findChild<QLabel*>("sourceBanner");
    EXPECT_EQ(sources->count(), 3);
    sources->setCurrentIndex(1);
    EXPECT_TRUE(pumpUntil([&] {
        return banner->text().contains("Motor lógico v2") &&
               window.findChild<QLabel*>("simulationClock")->text().contains("Pausado");
    }));
    sources->setCurrentIndex(2);
    EXPECT_TRUE(pumpUntil([&] {
        return banner->text().contains("Motor concurrente v2") &&
               window.findChild<QLabel*>("simulationClock")->text().contains("Pausado");
    }));
    sources->setCurrentIndex(0);
    EXPECT_TRUE(banner->text().contains("DEMOSTRACIÓN CON FIXTURES"));
}
void addThroughDialog(gui::MainWindow& window, int pid) {
    QTimer::singleShot(0, [&window, pid] {
        auto* dialog = dynamic_cast<gui::NewProcessDialog*>(
            QApplication::activeModalWidget());
        EXPECT_TRUE(dialog != nullptr);
        if (!dialog) return;
        dialog->findChild<QLineEdit*>("inputPid")->setText(QString::number(pid));
        dialog->findChild<QLineEdit*>("inputName")->setText("proceso real");
        dialog->findChild<QLineEdit*>("inputBytes")->setText("32");
        dialog->findChild<QLineEdit*>("inputCpu")->setText("15");
        dialog->accept();
    });
    button(window, "addProcess")->click();
}
void checkGuiRealEngines() {
    for (int engineIndex : {1, 2}) {
        gui::MainWindow window;
        auto* sources = window.findChild<QComboBox*>("engineSource");
        auto* table = dynamic_cast<gui::ProcessTable*>(
            window.findChild<QTableView*>("processTable"));
        auto* timeline = findWidget<gui::TimelineWidget>(window);
        auto* memory = findWidget<gui::MemoryWidget>(window);
        auto* events = dynamic_cast<gui::EventLogWidget*>(
            window.findChild<QTableView*>("eventLog"));
        auto* clock = window.findChild<QLabel*>("simulationClock");
        sources->setCurrentIndex(engineIndex);
        EXPECT_TRUE(pumpUntil([&] { return clock->text().contains("Pausado"); }));

        addThroughDialog(window, 200 + engineIndex);
        EXPECT_TRUE(pumpUntil([&] {
            return table->presentationModel()->rowCount() == 1 &&
                   table->presentationModel()->process(0)->estado == gui::api::Estado::READY;
        }));
        EXPECT_EQ(table->presentationModel()->process(0)->prioridad,
                  gui::api::Prioridad::P3);
        EXPECT_TRUE(memory->toolTip().contains("32 bytes"));
        const auto timelineRevision = timeline->revision();
        button(window, "step")->click();
        EXPECT_TRUE(pumpUntil([&] {
            return table->presentationModel()->process(0)->estado == gui::api::Estado::FINISH;
        }));
        EXPECT_TRUE(clock->text().contains("15 ms"));
        EXPECT_TRUE(timeline->revision() > timelineRevision);
        EXPECT_TRUE(memory->toolTip().isEmpty());
        EXPECT_TRUE(events->presentationModel()->rowCount() >= 5);

        button(window, "reset")->click();
        EXPECT_TRUE(pumpUntil([&] {
            return table->presentationModel()->rowCount() == 0 &&
                   clock->text().contains("0 ms");
        }));
        addThroughDialog(window, 300 + engineIndex);
        EXPECT_TRUE(pumpUntil([&] {
            return table->presentationModel()->rowCount() == 1 &&
                   table->presentationModel()->process(0)->estado == gui::api::Estado::READY;
        }));
        table->selectRow(0);
        auto* cancel = button(window, "cancelProcess");
        EXPECT_TRUE(cancel->isEnabled());
        cancel->click();
        EXPECT_TRUE(pumpUntil([&] {
            return table->presentationModel()->process(0)->estado == gui::api::Estado::CANCEL;
        }));
        EXPECT_TRUE(memory->toolTip().isEmpty());
    }
}
void checkSession(bool failInitially) {
    gui::MainWindow window;
    auto engine = std::make_unique<RecordingEngine>();
    auto* recording = engine.get();
    recording->fail = failInitially;
    window.attachEngine(std::move(engine), "Doble de transporte");
    auto* retry = button(window, "retrySnapshot");
    auto* timer = window.findChild<QTimer*>("snapshotRefresh");
    auto* cancel = button(window, "cancelProcess");
    auto* step = button(window, "step");
    auto* table = dynamic_cast<gui::ProcessTable*>(window.findChild<QTableView*>("processTable"));
    auto* clock = window.findChild<QLabel*>("simulationClock");
    auto* status = window.findChild<QLabel*>("actionStatus");
    EXPECT_TRUE(pumpUntil([&] { return recording->reads > 0 && timer->isActive(); }));
    EXPECT_TRUE(recording->readOffUiThread);
    EXPECT_TRUE(timer->isSingleShot());
    EXPECT_TRUE(timer->isActive());
    if (failInitially) {
        EXPECT_FALSE(step->isEnabled());
        EXPECT_EQ(timer->interval(), 300);
        EXPECT_EQ(table->model()->rowCount(), 0);
        EXPECT_TRUE(clock->text().contains("Sin snapshot"));
        recording->fail = false;
        refreshAndWait(window, *recording);
    }
    EXPECT_EQ(timer->interval(), 150);
    EXPECT_TRUE(step->isEnabled());
    EXPECT_FALSE(cancel->isEnabled());
    cancel->click();
    EXPECT_EQ(recording->cancellations, 0);
    table->selectRow(0);
    EXPECT_TRUE(cancel->isEnabled());
    cancel->click();
    EXPECT_TRUE(pumpUntil([&] { return recording->cancellations == 1; }));
    EXPECT_EQ(recording->cancellations, 1);
    table->clearSelection();
    EXPECT_EQ(table->selectedPid(), -1); // currentIndex puede seguir apuntando a una fila.
    EXPECT_FALSE(cancel->isEnabled());
    table->selectRow(0);
    recording->setFirstState(gui::api::Estado::FINISH);
    refreshAndWait(window, *recording);
    EXPECT_EQ(table->selectedPid(), 1);
    EXPECT_FALSE(cancel->isEnabled());
    cancel->click();
    EXPECT_EQ(recording->cancellations, 1);
    recording->setFirstState(gui::api::Estado::CANCEL);
    refreshAndWait(window, *recording);
    EXPECT_FALSE(cancel->isEnabled());
    recording->setFirstState(gui::api::Estado::WAIT);
    refreshAndWait(window, *recording);
    EXPECT_TRUE(cancel->isEnabled());

    const auto revision = table->presentationModel()->revision();
    const auto time = recording->stateCopy().tiempoSimuladoMs;
    for (int i = 0; i < 20; ++i) refreshAndWait(window, *recording);
    EXPECT_EQ(table->presentationModel()->revision(), revision);
    EXPECT_EQ(recording->steps, 0);
    EXPECT_EQ(recording->stateCopy().tiempoSimuladoMs, time);
    const auto visibleClock = clock->text();
    recording->fail = true;
    refreshAndWait(window, *recording);
    EXPECT_TRUE(timer->isActive());
    EXPECT_EQ(timer->interval(), 300);
    EXPECT_FALSE(cancel->isEnabled());
    EXPECT_FALSE(step->isEnabled());
    EXPECT_FALSE(button(window, "reset")->isEnabled());
    EXPECT_EQ(clock->text(), visibleClock);
    EXPECT_TRUE(status->text().contains("Fallo de lectura"));
    // Disparar la señal del timer prueba la ruta automática sin esperar wall-clock.
    int previousReads = recording->reads;
    EXPECT_TRUE(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection));
    EXPECT_TRUE(pumpUntil([&] { return recording->reads > previousReads; }));
    EXPECT_TRUE(pumpUntil([&] { return timer->interval() == 600; }));
    EXPECT_EQ(timer->interval(), 600);
    recording->fail = false;
    previousReads = recording->reads;
    EXPECT_TRUE(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection));
    EXPECT_TRUE(pumpUntil([&] { return recording->reads > previousReads; }));
    EXPECT_TRUE(pumpUntil([&] { return timer->interval() == 150; }));
    EXPECT_EQ(timer->interval(), 150);
    EXPECT_TRUE(step->isEnabled());
    EXPECT_TRUE(cancel->isEnabled());
    EXPECT_EQ(table->selectedPid(), 1);
    EXPECT_TRUE(status->text().contains("recuperada"));
    EXPECT_EQ(recording->stateCopy().tiempoSimuladoMs, time);
    EXPECT_EQ(recording->steps, 0);
    recording->setInvalidTimeline(true); // Bloque fuera del reloj publicado.
    refreshAndWait(window, *recording);
    EXPECT_EQ(timer->interval(), 300);
    EXPECT_FALSE(step->isEnabled());
    EXPECT_EQ(clock->text(), visibleClock);
    recording->setInvalidTimeline(false);
    refreshAndWait(window, *recording);
    EXPECT_EQ(timer->interval(), 150);
    EXPECT_TRUE(step->isEnabled());
    step->click();
    auto* reset = button(window, "reset");
    EXPECT_TRUE(pumpUntil([&] {
        return recording->steps == 1 && reset->isEnabled();
    }));
    EXPECT_EQ(recording->steps, 1);
    reset->click();
    EXPECT_TRUE(pumpUntil([&] { return recording->resets == 1; }));
    EXPECT_EQ(recording->resets, 1);
    EXPECT_FALSE(button(window, "start")->isEnabled());
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    checkPreview();
    checkForm();
    checkFactories();
    checkGuiRealEngines();
    checkSession(false);
    checkSession(true);
    return test::finish();
}
