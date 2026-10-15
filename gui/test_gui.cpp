#include "gui/MainWindow.h"
#include "gui/NewProcessDialog.h"
#include "gui/ProcessTable.h"
#include "gui/PreviewFrames.h"
#include "gui/TimelineWidget.h"
#include "tests/TestSupport.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include <stdexcept>

namespace {
// Doble de transporte: no ejecuta procesos ni modifica el reloj.
class RecordingEngine : public gui::EngineSession::Engine {
public:
    gui::api::EstadoSistema state = gui::previewScenarios().front().frames.front().state;
    mutable int reads = 0;
    int steps = 0, adds = 0, resets = 0, cancellations = 0;
    bool fail = false;
    gui::api::Resultado agregarProceso(const gui::api::AltaProceso&) override { ++adds; return {}; }
    gui::api::Resultado cancelarProceso(int pid) override { ++cancellations; return {gui::api::CodigoResultado::OK, pid}; }
    gui::api::Resultado iniciar() override { throw std::logic_error("No se permite continuidad"); }
    gui::api::Resultado pausar() override { return {}; }
    gui::api::Resultado reiniciar() override { ++resets; return {}; }
    gui::api::ResultadoPaso siguientePaso() override { ++steps; return {}; }
    gui::api::EstadoSistema obtenerEstado() const override {
        EXPECT_TRUE(QThread::currentThread() == qApp->thread());
        ++reads;
        if (fail) throw std::runtime_error("Fallo de lectura controlado");
        return state;
    }
};
QPushButton* button(gui::MainWindow& window, const char* name) { return window.findChild<QPushButton*>(name); }
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
    EXPECT_TRUE(timer->isSingleShot());
    EXPECT_TRUE(timer->isActive());
    if (failInitially) {
        EXPECT_FALSE(step->isEnabled());
        EXPECT_EQ(timer->interval(), 300);
        EXPECT_EQ(table->model()->rowCount(), 0);
        EXPECT_TRUE(clock->text().contains("Sin snapshot"));
        recording->fail = false;
        retry->click();
    }
    EXPECT_EQ(timer->interval(), 150);
    EXPECT_TRUE(step->isEnabled());
    EXPECT_FALSE(cancel->isEnabled());
    cancel->click();
    EXPECT_EQ(recording->cancellations, 0);
    table->selectRow(0);
    EXPECT_TRUE(cancel->isEnabled());
    cancel->click();
    EXPECT_EQ(recording->cancellations, 1);
    table->clearSelection();
    EXPECT_EQ(table->selectedPid(), -1); // currentIndex puede seguir apuntando a una fila.
    EXPECT_FALSE(cancel->isEnabled());
    table->selectRow(0);
    recording->state.procesos[0].estado = gui::api::Estado::FINISH;
    retry->click();
    EXPECT_EQ(table->selectedPid(), 1);
    EXPECT_FALSE(cancel->isEnabled());
    cancel->click();
    EXPECT_EQ(recording->cancellations, 1);
    recording->state.procesos[0].estado = gui::api::Estado::CANCEL;
    retry->click();
    EXPECT_FALSE(cancel->isEnabled());
    recording->state.procesos[0].estado = gui::api::Estado::WAIT;
    retry->click();
    EXPECT_TRUE(cancel->isEnabled());

    const auto revision = table->presentationModel()->revision();
    const auto time = recording->state.tiempoSimuladoMs;
    for (int i = 0; i < 20; ++i) retry->click();
    EXPECT_EQ(table->presentationModel()->revision(), revision);
    EXPECT_EQ(recording->steps, 0);
    EXPECT_EQ(recording->state.tiempoSimuladoMs, time);
    const auto visibleClock = clock->text();
    recording->fail = true;
    retry->click();
    EXPECT_TRUE(timer->isActive());
    EXPECT_EQ(timer->interval(), 300);
    EXPECT_FALSE(cancel->isEnabled());
    EXPECT_FALSE(step->isEnabled());
    EXPECT_FALSE(button(window, "reset")->isEnabled());
    EXPECT_EQ(clock->text(), visibleClock);
    EXPECT_TRUE(status->text().contains("Fallo de lectura"));
    // Disparar la señal del timer prueba la ruta automática sin esperar wall-clock.
    EXPECT_TRUE(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection));
    EXPECT_EQ(timer->interval(), 600);
    recording->fail = false;
    EXPECT_TRUE(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection));
    EXPECT_EQ(timer->interval(), 150);
    EXPECT_TRUE(step->isEnabled());
    EXPECT_TRUE(cancel->isEnabled());
    EXPECT_EQ(table->selectedPid(), 1);
    EXPECT_TRUE(status->text().contains("recuperada"));
    EXPECT_EQ(recording->state.tiempoSimuladoMs, time);
    EXPECT_EQ(recording->steps, 0);
    recording->state.timeline.emplace_back(1, 0, 1); // Bloque fuera del reloj publicado.
    retry->click();
    EXPECT_EQ(timer->interval(), 300);
    EXPECT_FALSE(step->isEnabled());
    EXPECT_EQ(clock->text(), visibleClock);
    recording->state.timeline.clear();
    retry->click();
    EXPECT_EQ(timer->interval(), 150);
    EXPECT_TRUE(step->isEnabled());
    step->click();
    EXPECT_EQ(recording->steps, 1);
    button(window, "reset")->click();
    EXPECT_EQ(recording->resets, 1);
    EXPECT_FALSE(button(window, "start")->isEnabled());
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    checkPreview();
    checkForm();
    checkSession(false);
    checkSession(true);
    return test::finish();
}
