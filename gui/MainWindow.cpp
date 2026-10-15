#include "gui/MainWindow.h"
#include "gui/CPUWidget.h"
#include "gui/EventLogWidget.h"
#include "gui/MemoryWidget.h"
#include "gui/NewProcessDialog.h"
#include "gui/ProcessTable.h"
#include "gui/QueueWidget.h"
#include "gui/TimelineWidget.h"
#include "gui/WidgetSupport.h"
#include <QApplication>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
namespace gui {
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), frames_(previewFrames()) {
    setWindowTitle("ConcurrentSimulator · Poma · prototipo de presentación");
    resize(1280, 850);
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QVBoxLayout(central);
    auto* heading = new QLabel("ConcurrentSimulator  /  CPU lógico único", this);
    heading->setStyleSheet("font-size: 22px; font-weight: 600; padding: 8px 0;");
    root->addWidget(heading);
    banner_ = new QLabel(this);
    banner_->setObjectName("sourceBanner");
    banner_->setWordWrap(true);
    banner_->setStyleSheet("background: #fff0cb; color: #493800; padding: 10px;");
    root->addWidget(banner_);
    auto* controls = new QHBoxLayout;
    sources_ = new QComboBox(this);
    sources_->addItems({"Vista previa de snapshots", "Motor lógico · pendiente", "Motor concurrente · pendiente"});
    auto* model = qobject_cast<QStandardItemModel*>(sources_->model());
    model->item(1)->setEnabled(false);
    model->item(2)->setEnabled(false);
    sources_->setToolTip("Los motores se habilitan al integrar sus fábricas y ratificar la API.");
    controls->addWidget(sources_);
    auto button = [this, controls](const char* id, const QString& text) {
        auto* result = new QPushButton(text, this);
        result->setObjectName(id);
        controls->addWidget(result);
        return result;
    };
    previous_ = button("previousFrame", "Anterior");
    start_ = button("start", "Reproducir vistas");
    pause_ = button("pause", "Pausar vistas");
    step_ = button("step", "Vista siguiente");
    reset_ = button("reset", "Volver al inicio");
    add_ = button("addProcess", "Formulario de alta");
    cancel_ = button("cancelProcess", "Cancelar PID seleccionado");
    root->addLayout(controls);
    clock_ = new QLabel(this);
    clock_->setObjectName("simulationClock");
    root->addWidget(clock_);
    auto* middle = new QSplitter(Qt::Horizontal, this);
    auto group = [middle](const QString& title, QWidget* content) {
        auto* box = new QGroupBox(title, middle);
        auto* layout = new QVBoxLayout(box);
        layout->addWidget(content);
        middle->addWidget(box);
    };
    cpu_ = new CPUWidget(this);
    queue_ = new QueueWidget(this);
    memory_ = new MemoryWidget(this);
    group("CPU", cpu_);
    group("READY · orden recibido", queue_);
    group("Memoria", memory_);
    middle->setSizes({300, 450, 350});
    middle->setStretchFactor(0, 3);
    middle->setStretchFactor(1, 4);
    middle->setStretchFactor(2, 3);
    root->addWidget(middle);
    processes_ = new ProcessTable(this);
    events_ = new EventLogWidget(this);
    auto* tabs = new QTabWidget(this);
    tabs->addTab(processes_, "Procesos e historial");
    tabs->addTab(events_, "Eventos de la sesión");
    root->addWidget(tabs, 2);
    auto* timelineHeading = new QHBoxLayout;
    timelineHeading->addWidget(new QLabel("Timeline · ms simulados · rayado = idle inferido", this));
    auto* zoom = new QSlider(Qt::Horizontal, this);
    zoom->setObjectName("timelineZoom");
    zoom->setRange(1, 80);
    zoom->setValue(12);
    timelineHeading->addWidget(new QLabel("Zoom", this));
    timelineHeading->addWidget(zoom);
    root->addLayout(timelineHeading);
    timeline_ = new TimelineWidget(this);
    root->addWidget(timeline_, 1);
    connect(zoom, &QSlider::valueChanged, timeline_, &TimelineWidget::setZoom);
    auto* speed = new QComboBox(this);
    speed->setObjectName("visualSpeed");
    speed->addItem("Vistas: cada 1.5 s", 1500);
    speed->addItem("Vistas: cada 0.7 s", 700);
    speed->addItem("Vistas: cada 3 s", 3000);
    root->addWidget(speed);
    status_ = new QLabel(this);
    status_->setObjectName("actionStatus");
    status_->setWordWrap(true);
    root->addWidget(status_);
    previewTimer_ = new QTimer(this);
    previewTimer_->setInterval(1500);
    refreshTimer_ = new QTimer(this);
    refreshTimer_->setInterval(150);
    connect(previewTimer_, &QTimer::timeout, this, &MainWindow::advancePreview);
    connect(refreshTimer_, &QTimer::timeout, this, &MainWindow::refreshEngine);
    connect(speed, &QComboBox::currentIndexChanged, this, [this, speed] {
        previewTimer_->setInterval(speed->currentData().toInt());
    });
    connect(previous_, &QPushButton::clicked, this, [this] {
        previewTimer_->stop();
        if (frame_ > 0) --frame_;
        showFrame();
    });
    connect(start_, &QPushButton::clicked, this, [this] {
        if (!session_) { previewTimer_->start(); showFrame(); }
    });
    connect(pause_, &QPushButton::clicked, this, [this] {
        if (session_) action([this] { report(session_->engine().pausar().codigo); refreshEngine(); });
        else { previewTimer_->stop(); showFrame(); }
    });
    connect(step_, &QPushButton::clicked, this, [this] {
        if (session_) action([this] { report(session_->engine().siguientePaso().codigo); refreshEngine(); });
        else { previewTimer_->stop(); advancePreview(); }
    });
    connect(reset_, &QPushButton::clicked, this, [this] {
        if (session_) action([this] { report(session_->engine().reiniciar().codigo); refreshEngine(); });
        else { previewTimer_->stop(); frame_ = 0; showFrame(); }
    });
    connect(add_, &QPushButton::clicked, this, &MainWindow::addProcess);
    connect(cancel_, &QPushButton::clicked, this, [this] {
        if (session_) action([this] {
            report(session_->engine().cancelarProceso(processes_->selectedPid()).codigo);
            refreshEngine();
        });
    });
    showFrame();
}
void MainWindow::present(const api::EstadoSistema& state) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    clock_->setText(QString("Reloj publicado: %1 ms simulados  ·  Estado: %2")
        .arg(number(state.tiempoSimuladoMs), executionName(state.estadoEjecucion)));
    processes_->present(state);
    queue_->present(state);
    cpu_->present(state);
    memory_->present(state);
    timeline_->present(state);
    events_->present(state);
    updateControls(state);
}
void MainWindow::updateControls(const api::EstadoSistema& state) {
    const bool preview = !session_;
    const bool paused = state.estadoEjecucion == api::EstadoEjecucion::PAUSADO;
    previous_->setVisible(preview);
    previous_->setEnabled(frame_ > 0);
    start_->setEnabled(preview && frame_ + 1 < frames_.size() && !previewTimer_->isActive());
    pause_->setEnabled(preview ? previewTimer_->isActive() : state.estadoEjecucion == api::EstadoEjecucion::EJECUTANDO);
    step_->setEnabled(preview ? frame_ + 1 < frames_.size() : paused);
    cancel_->setEnabled(!preview);
}
void MainWindow::showFrame() {
    banner_->setText(QString("DEMOSTRACIÓN CON FIXTURES · sin motor conectado · %1/%2 · %3")
        .arg(frame_ + 1).arg(frames_.size()).arg(QString::fromStdString(frames_[frame_].title)));
    present(frames_[frame_].state);
}
void MainWindow::advancePreview() {
    if (frame_ + 1 < frames_.size()) ++frame_;
    if (frame_ + 1 == frames_.size()) previewTimer_->stop();
    showFrame();
}
void MainWindow::action(const std::function<void()>& operation) {
    try { operation(); }
    catch (const std::exception& e) {
        refreshTimer_->stop();
        status_->setText("Error del adaptador: " + QString::fromUtf8(e.what()));
    }
}
void MainWindow::report(api::CodigoResultado result) { status_->setText(resultName(result)); }
void MainWindow::refreshEngine() {
    if (session_) action([this] { present(session_->snapshot()); });
}
void MainWindow::attachEngine(std::unique_ptr<EngineSession::Engine> engine, const QString& label) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    previewTimer_->stop();
    refreshTimer_->stop();
    session_ = std::make_unique<EngineSession>(std::move(engine));
    sources_->clear();
    sources_->addItem(label);
    sources_->setEnabled(false);
    banner_->setText("Adaptador experimental API v2 candidata · " + label);
    start_->setText("Inicio continuo pendiente");
    start_->setToolTip("Driver continuo por ratificar: el refresco solo consulta snapshots.");
    step_->setText("Siguiente paso");
    pause_->setText("Pausar");
    reset_->setText("Reiniciar");
    add_->setText("Agregar proceso");
    refreshEngine();
    refreshTimer_->start();
}
void MainWindow::addProcess() {
    NewProcessDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;
    if (session_) action([this, &dialog] {
        report(session_->engine().agregarProceso(dialog.input()).codigo);
        refreshEngine();
    });
    else status_->setText("Formulario válido. No se admitió ningún proceso: la vista previa no es un motor.");
}
}
