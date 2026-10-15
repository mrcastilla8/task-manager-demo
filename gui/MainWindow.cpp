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
#include <QScrollBar>
#include <QPushButton>
#include <QSlider>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
namespace gui {
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
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
    scenarios_ = new QComboBox(this);
    scenarios_->setObjectName("previewScenario");
    for (const auto& scenario : playback_.scenarios()) scenarios_->addItem(QString::fromStdString(scenario.title));
    root->addWidget(scenarios_);
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
    retryButton_ = new QPushButton("Reintentar lectura del snapshot", this);
    retryButton_->setObjectName("retrySnapshot");
    retryButton_->setVisible(false);
    root->addWidget(retryButton_);
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
    refreshTimer_->setObjectName("snapshotRefresh");
    refreshTimer_->setSingleShot(true);
    refreshTimer_->setInterval(150);
    connect(previewTimer_, &QTimer::timeout, this, &MainWindow::advancePreview);
    connect(refreshTimer_, &QTimer::timeout, this, &MainWindow::refreshEngine);
    connect(retryButton_, &QPushButton::clicked, this, [this] {
        refreshTimer_->stop();
        refreshEngine();
    });
    connect(processes_->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] { updateCancellation(); });
    connect(scenarios_, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (session_ || index < 0) return;
        previewTimer_->stop();
        playback_.select(static_cast<std::size_t>(index));
        clearVisualContext();
        showFrame();
    });
    connect(speed, &QComboBox::currentIndexChanged, this, [this, speed] {
        previewTimer_->setInterval(speed->currentData().toInt());
    });
    connect(previous_, &QPushButton::clicked, this, [this] {
        previewTimer_->stop();
        playback_.previous();
        showFrame();
    });
    connect(start_, &QPushButton::clicked, this, [this] {
        if (!session_ && !playback_.atEnd()) { previewTimer_->start(); showFrame(); }
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
        else { previewTimer_->stop(); playback_.reset(); clearVisualContext(); showFrame(); }
    });
    connect(add_, &QPushButton::clicked, this, &MainWindow::addProcess);
    connect(cancel_, &QPushButton::clicked, this, [this] {
        if (session_ && snapshotHealthy_ && processes_->selectedIsActive()) action([this] {
            report(session_->engine().cancelarProceso(processes_->selectedPid()).codigo);
            refreshEngine();
        });
    });
    showFrame();
}
void MainWindow::present(const api::EstadoSistema& state) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    timeline_->present(state); // Valida antes de modificar el resto de vistas.
    clock_->setText(QString("Reloj publicado: %1 ms simulados  ·  Estado: %2")
        .arg(number(state.tiempoSimuladoMs), executionName(state.estadoEjecucion)));
    processes_->present(state);
    queue_->present(state);
    cpu_->present(state);
    memory_->present(state);
    events_->present(state);
    updateControls(state);
}
void MainWindow::updateControls(const api::EstadoSistema& state) {
    const bool preview = !session_;
    const bool paused = state.estadoEjecucion == api::EstadoEjecucion::PAUSADO;
    previous_->setVisible(preview);
    previous_->setEnabled(playback_.frameIndex() > 0);
    start_->setEnabled(preview && !playback_.atEnd() && !previewTimer_->isActive());
    pause_->setEnabled(preview ? previewTimer_->isActive() : snapshotHealthy_ && state.estadoEjecucion == api::EstadoEjecucion::EJECUTANDO);
    step_->setEnabled(preview ? !playback_.atEnd() : snapshotHealthy_ && paused);
    add_->setEnabled(preview || snapshotHealthy_);
    reset_->setEnabled(preview || snapshotHealthy_);
    updateCancellation();
}
void MainWindow::updateCancellation() {
    cancel_->setEnabled(session_ && snapshotHealthy_ && processes_->selectedIsActive());
}
void MainWindow::clearVisualContext() {
    lastSnapshot_.reset();
    api::EstadoSistema empty;
    processes_->clearSelection();
    processes_->present(empty);
    events_->present(empty);
    timeline_->present(empty);
    cpu_->present(empty);
    queue_->present(empty);
    memory_->present(empty);
    clock_->setText("Sin snapshot de la sesión actual");
    processes_->verticalScrollBar()->setValue(0);
    processes_->horizontalScrollBar()->setValue(0);
    events_->verticalScrollBar()->setValue(0);
    events_->horizontalScrollBar()->setValue(0);
    timeline_->resetContext();
}
void MainWindow::showFrame() {
    banner_->setText(QString("DEMOSTRACIÓN CON FIXTURES · sin motor conectado · %1 · vista %2/%3 · %4")
        .arg(scenarios_->currentText()).arg(playback_.frameIndex() + 1).arg(playback_.frameCount())
        .arg(QString::fromStdString(playback_.current().title)));
    present(playback_.current().state);
}
void MainWindow::advancePreview() {
    playback_.next();
    if (playback_.atEnd()) previewTimer_->stop();
    showFrame();
}
void MainWindow::action(const std::function<void()>& operation) {
    try { operation(); }
    catch (const std::exception& e) {
        status_->setText("Error del adaptador: " + QString::fromUtf8(e.what()));
    }
}
void MainWindow::report(api::CodigoResultado result) { status_->setText(resultName(result)); }
void MainWindow::refreshEngine() {
    if (!session_) return;
    try {
        auto state = session_->snapshot();
        const bool recovered = !retry_.healthy();
        snapshotHealthy_ = true;
        if (!lastSnapshot_ || !sameSnapshot(*lastSnapshot_, state)) {
            present(state);
            lastSnapshot_ = std::move(state);
        }
        retry_.success();
        if (recovered) status_->setText("Lectura recuperada. Refresco normal restablecido.");
        updateControls(*lastSnapshot_);
    } catch (const std::exception& e) {
        snapshotHealthy_ = false;
        retry_.failure();
        step_->setEnabled(false);
        pause_->setEnabled(false);
        add_->setEnabled(false);
        reset_->setEnabled(false);
        updateCancellation();
        status_->setText(QString("Snapshot no disponible: %1. Última vista conservada; reintento en %2 ms o con el botón.")
            .arg(QString::fromUtf8(e.what())).arg(retry_.delayMs()));
    }
    refreshTimer_->start(retry_.delayMs());
}
void MainWindow::attachEngine(std::unique_ptr<EngineSession::Engine> engine, const QString& label) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    previewTimer_->stop();
    refreshTimer_->stop();
    session_ = std::make_unique<EngineSession>(std::move(engine));
    snapshotHealthy_ = false;
    retry_.success();
    clearVisualContext();
    scenarios_->setVisible(false);
    retryButton_->setVisible(true);
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
}
void MainWindow::addProcess() {
    if (session_ && !snapshotHealthy_) return;
    NewProcessDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;
    if (session_) action([this, &dialog] {
        report(session_->engine().agregarProceso(dialog.input()).codigo);
        refreshEngine();
    });
    else status_->setText("Formulario válido. No se admitió ningún proceso: la vista previa no es un motor.");
}
}
