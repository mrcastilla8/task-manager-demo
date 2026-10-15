#include "gui/MainWindow.h"
#include "gui/CPUWidget.h"
#include "gui/NewProcessDialog.h"
#include "gui/ProcessTable.h"
#include "gui/QueueWidget.h"
#include "gui/EventLogWidget.h"
#include "gui/TimelineWidget.h"
#include "gui/PreviewFrames.h"
#include "tests/TestSupport.h"
#include <QApplication>
#include <QCheckBox>
#include <QGraphicsScene>
#include <QFontDatabase>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <stdexcept>

namespace {
// Doble de transporte únicamente para comprobar delegación, no simula scheduling.
class RecordingEngine : public gui::EngineSession::Engine {
public:
    gui::api::EstadoSistema state = gui::previewFrames().front().state;
    int reads = 0, steps = 0, adds = 0, resets = 0, cancelled = -1;
    bool rejectSnapshot = false;
    gui::api::CodigoResultado cancelResult = gui::api::CodigoResultado::PID_INEXISTENTE;
    gui::api::Resultado agregarProceso(const gui::api::AltaProceso&) override {
        ++adds; return {gui::api::CodigoResultado::MEMORIA_INSUFICIENTE, -1};
    }
    gui::api::Resultado cancelarProceso(int pid) override { cancelled = pid; return {cancelResult, pid}; }
    gui::api::Resultado iniciar() override { throw std::logic_error("GUI no debe impulsar continuidad"); }
    gui::api::Resultado pausar() override { return {}; }
    gui::api::Resultado reiniciar() override { ++resets; return {}; }
    gui::api::ResultadoPaso siguientePaso() override { ++steps; return {}; }
    gui::api::EstadoSistema obtenerEstado() const override {
        EXPECT_TRUE(QThread::currentThread() == qApp->thread());
        ++const_cast<RecordingEngine*>(this)->reads;
        if (rejectSnapshot) throw std::runtime_error("Fallo de snapshot controlado");
        return state;
    }
};
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
#ifdef Q_OS_WIN
    // Offscreen en Windows no siempre descubre las fuentes del sistema.
    const int fontId = QFontDatabase::addApplicationFont(qEnvironmentVariable("WINDIR") + "/Fonts/segoeui.ttf");
    if (fontId >= 0) app.setFont(QFont(QFontDatabase::applicationFontFamilies(fontId).first(), 9));
#endif
    gui::MainWindow window;
    window.show();
    QTest::qWait(20);
    auto* table = window.findChild<QTableWidget*>("processTable");
    auto* queue = window.findChild<QListWidget*>("readyQueue");
    auto* clock = window.findChild<QLabel*>("simulationClock");
    auto* next = window.findChild<QPushButton*>("step");
    EXPECT_TRUE(table && queue && clock && next);
    EXPECT_EQ(table->rowCount(), 3);
    EXPECT_TRUE(queue->item(0)->text().contains("PID 2"));
    const QString initialClock = clock->text();
    QTest::qWait(200);
    EXPECT_EQ(clock->text(), initialClock);
    QTest::mouseClick(next, Qt::LeftButton);
    EXPECT_TRUE(window.findChild<QLabel*>("cpuDescription")->text().contains("PID 2"));
    EXPECT_EQ(clock->text(), initialClock); // RUN publicado en el mismo t=0.
    QTest::mouseClick(next, Qt::LeftButton);
    EXPECT_TRUE(clock->text().contains("4 ms"));
    auto* zoom = dynamic_cast<gui::TimelineWidget*>(window.findChild<QGraphicsView*>("timeline"));
    zoom->setZoom(70);
    EXPECT_TRUE(clock->text().contains("4 ms"));
    auto* log = window.findChild<QTableWidget*>("eventLog");
    EXPECT_EQ(log->rowCount(), 5);

    auto reclassifiedRun = gui::previewFrames()[1].state;
    reclassifiedRun.procesos[1].prioridad = gui::api::Prioridad::P1;
    gui::CPUWidget cpuView;
    cpuView.present(reclassifiedRun);
    EXPECT_TRUE(cpuView.findChild<QLabel*>("cpuDescription")->text().contains("cuota fijada: 30 ms"));
    gui::ProcessTable processView;
    processView.present(reclassifiedRun);
    EXPECT_EQ(processView.item(1, 5)->text(), QString("10"));

    gui::NewProcessDialog dialog;
    dialog.accept();
    EXPECT_TRUE(!dialog.findChild<QLabel*>("inputError")->text().isEmpty());
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

    auto engine = std::make_unique<RecordingEngine>();
    auto* recording = engine.get();
    window.attachEngine(std::move(engine), "Doble de prueba · sin ejecución real");
    const int reads = recording->reads;
    QTest::qWait(350);
    EXPECT_TRUE(recording->reads > reads);
    EXPECT_EQ(recording->steps, 0);
    EXPECT_EQ(recording->state.tiempoSimuladoMs, 0);
    EXPECT_FALSE(window.findChild<QPushButton*>("start")->isEnabled());
    QTest::mouseClick(next, Qt::LeftButton);
    EXPECT_EQ(recording->steps, 1);
    QTimer::singleShot(0, &window, [&window] {
        auto* modal = dynamic_cast<gui::NewProcessDialog*>(window.findChild<QDialog*>());
        EXPECT_TRUE(modal != nullptr);
        if (modal) {
            modal->findChild<QLineEdit*>("inputName")->setText("Alta de prueba");
            modal->accept();
        }
    });
    QTest::mouseClick(window.findChild<QPushButton*>("addProcess"), Qt::LeftButton);
    EXPECT_EQ(recording->adds, 1);
    EXPECT_EQ(recording->state.procesos.size(), 3U);
    EXPECT_TRUE(window.findChild<QLabel*>("actionStatus")->text().contains("Memoria insuficiente"));
    table->selectRow(0);
    QTest::mouseClick(window.findChild<QPushButton*>("cancelProcess"), Qt::LeftButton);
    EXPECT_EQ(recording->cancelled, 1);
    EXPECT_TRUE(window.findChild<QLabel*>("actionStatus")->text().contains("no existe"));
    recording->cancelResult = gui::api::CodigoResultado::PID_FINALIZADO;
    QTest::mouseClick(window.findChild<QPushButton*>("cancelProcess"), Qt::LeftButton);
    EXPECT_TRUE(window.findChild<QLabel*>("actionStatus")->text().contains("finalizó"));
    QTest::mouseClick(window.findChild<QPushButton*>("reset"), Qt::LeftButton);
    EXPECT_EQ(recording->resets, 1);
    recording->rejectSnapshot = true;
    QTest::qWait(200);
    EXPECT_TRUE(window.findChild<QLabel*>("actionStatus")->text().contains("Fallo de snapshot"));

    const QString screenshot = qEnvironmentVariable("SIMULATOR_GUI_SCREENSHOT");
    if (!screenshot.isEmpty()) {
        gui::MainWindow preview;
        preview.show();
        QTest::qWait(20);
        EXPECT_TRUE(preview.grab().save(screenshot));
        auto* advance = preview.findChild<QPushButton*>("step");
        for (int i = 0; i < 3; ++i) QTest::mouseClick(advance, Qt::LeftButton);
        EXPECT_TRUE(preview.grab().save(QFileInfo(screenshot).absolutePath() + "/preview-run.png"));
        for (int i = 0; i < 2; ++i) QTest::mouseClick(advance, Qt::LeftButton);
        EXPECT_TRUE(preview.grab().save(QFileInfo(screenshot).absolutePath() + "/preview-idle.png"));
    }
    return test::finish();
}
