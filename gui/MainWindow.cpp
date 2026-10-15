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
#include <QMetaObject>
#include <QPointer>
#include <QSlider>
#include <QSplitter>
#include <QTabWidget>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
namespace gui {
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("ConcurrentSimulator · simulación v2");
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
    sources_->setObjectName("engineSource");
    sources_->addItems({"Vista previa de snapshots", "Motor lógico v2", "Motor concurrente v2"});
    sources_->setToolTip("Cada motor crea una sesión real con Core y Scheduler v2.");
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
    connect(sources_, &QComboBox::currentIndexChanged, this, &MainWindow::selectSource);
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
        if (session_) action([this](auto callback) { session_->pausar(std::move(callback)); });
        else { previewTimer_->stop(); showFrame(); }
    });
    connect(step_, &QPushButton::clicked, this, [this] {
        if (session_) stepAction();
        else { previewTimer_->stop(); advancePreview(); }
    });
    connect(reset_, &QPushButton::clicked, this, [this] {
        if (session_) action([this](auto callback) { session_->reiniciarParaPasos(std::move(callback)); });
        else { previewTimer_->stop(); playback_.reset(); clearVisualContext(); showFrame(); }
    });
    connect(add_, &QPushButton::clicked, this, &MainWindow::addProcess);
    connect(cancel_, &QPushButton::clicked, this, [this] {
        if (session_ && snapshotHealthy_ && processes_->selectedIsActive()) {
            const int pid = processes_->selectedPid();
            action([this, pid](auto callback) { session_->cancelar(pid, std::move(callback)); });
        }
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
    pause_->setEnabled(preview ? previewTimer_->isActive() : false);
    step_->setEnabled(preview ? !playback_.atEnd() : snapshotHealthy_ && paused);
    add_->setEnabled(preview || snapshotHealthy_);
    reset_->setEnabled(preview || snapshotHealthy_);
    if (actionPending_) {
        step_->setEnabled(false);
        add_->setEnabled(false);
        reset_->setEnabled(false);
    }
    updateCancellation();
}
void MainWindow::updateCancellation() {
    cancel_->setEnabled(session_ && snapshotHealthy_ && !actionPending_ &&
                        processes_->selectedIsActive());
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
void MainWindow::action(const std::function<void(EngineSession::ResultCallback)>& operation) {
    if (!session_ || actionPending_) return;
    actionPending_ = true;
    if (lastSnapshot_) updateControls(*lastSnapshot_);
    const int generation = sessionGeneration_;
    QPointer<MainWindow> guard(this);
    try {
        operation([guard, generation](api::Resultado result, EngineSession::Error error) {
            if (!guard) return;
            QMetaObject::invokeMethod(guard.data(), [guard, generation, result, error] {
                if (guard) guard->finishAction(result.codigo, error, generation);
            }, Qt::QueuedConnection);
        });
    } catch (...) {
        finishAction(api::CodigoResultado::ESTADO_INVALIDO,
                     std::current_exception(), generation);
    }
}

void MainWindow::stepAction() {
    if (!session_ || actionPending_) return;
    actionPending_ = true;
    if (lastSnapshot_) updateControls(*lastSnapshot_);
    const int generation = sessionGeneration_;
    QPointer<MainWindow> guard(this);
    try {
        session_->siguientePaso([guard, generation](api::ResultadoPaso result,
                                                   EngineSession::Error error) {
            if (!guard) return;
            QMetaObject::invokeMethod(guard.data(), [guard, generation, result, error] {
                if (!guard || generation != guard->sessionGeneration_) return;
                guard->actionPending_ = false;
                if (error) {
                    try { std::rethrow_exception(error); }
                    catch (const std::exception& exception) {
                        guard->status_->setText("Error del motor: " + QString::fromUtf8(exception.what()));
                    }
                    catch (...) { guard->status_->setText("Error desconocido del motor"); }
                } else {
                    guard->report(result.codigo);
                }
                guard->refreshEngine();
            }, Qt::QueuedConnection);
        });
    } catch (...) {
        finishAction(api::CodigoResultado::ESTADO_INVALIDO,
                     std::current_exception(), generation);
    }
}

void MainWindow::finishAction(api::CodigoResultado result, EngineSession::Error error,
                              int generation) {
    if (generation != sessionGeneration_) return;
    actionPending_ = false;
    if (error) {
        try { std::rethrow_exception(error); }
        catch (const std::exception& exception) {
            status_->setText("Error del motor: " + QString::fromUtf8(exception.what()));
        }
        catch (...) { status_->setText("Error desconocido del motor"); }
    } else {
        report(result);
    }
    refreshEngine();
}

void MainWindow::selectSource(int index) {
    if (index == 0) {
        if (session_) {
            ++sessionGeneration_;
            refreshTimer_->stop();
            session_.reset();
            snapshotHealthy_ = false;
            snapshotPending_ = false;
            actionPending_ = false;
            retry_.success();
            scenarios_->setVisible(true);
            retryButton_->setVisible(false);
            start_->setText("Reproducir vistas");
            step_->setText("Vista siguiente");
            pause_->setText("Pausar vistas");
            reset_->setText("Volver al inicio");
            add_->setText("Formulario de alta");
            clearVisualContext();
            showFrame();
        }
        return;
    }
    const bool concurrente = index == 2;
    auto motor = concurrente
        ? EngineSession::crearConcurrente(1024)
        : EngineSession::crearLogico(1024);
    attachEngine(std::move(motor), concurrente ? "Motor concurrente v2"
                                               : "Motor lógico v2");
}
void MainWindow::report(api::CodigoResultado result) { status_->setText(resultName(result)); }
void MainWindow::refreshEngine() {
    if (!session_ || snapshotPending_) return;
    snapshotPending_ = true;
    const int generation = sessionGeneration_;
    QPointer<MainWindow> guard(this);
    try {
        session_->snapshot([guard, generation](api::EstadoSistema state,
                                              EngineSession::Error error) mutable {
            if (!guard) return;
            QMetaObject::invokeMethod(guard.data(),
                [guard, generation, state = std::move(state), error]() mutable {
                    if (!guard || generation != guard->sessionGeneration_) return;
                    guard->snapshotPending_ = false;
                    if (!error) {
                        const bool recovered = !guard->retry_.healthy();
                        try {
                            if (!guard->lastSnapshot_ ||
                                !sameSnapshot(*guard->lastSnapshot_, state)) {
                                guard->present(state);
                                guard->lastSnapshot_ = std::move(state);
                            }
                            guard->snapshotHealthy_ = true;
                            guard->retry_.success();
                            if (recovered) guard->status_->setText(
                                "Lectura recuperada. Refresco normal restablecido.");
                            guard->updateControls(*guard->lastSnapshot_);
                        } catch (const std::exception& exception) {
                            error = std::make_exception_ptr(exception);
                        }
                    }
                    if (error) {
                        guard->snapshotHealthy_ = false;
                        guard->retry_.failure();
                        guard->step_->setEnabled(false);
                        guard->pause_->setEnabled(false);
                        guard->add_->setEnabled(false);
                        guard->reset_->setEnabled(false);
                        guard->updateCancellation();
                        QString message = "Error desconocido";
                        try { std::rethrow_exception(error); }
                        catch (const std::exception& exception) {
                            message = QString::fromUtf8(exception.what());
                        } catch (...) {}
                        guard->status_->setText(QString("Snapshot no disponible: %1. Última vista conservada; reintento en %2 ms o con el botón.")
                            .arg(message).arg(guard->retry_.delayMs()));
                    }
                    if (guard->session_) guard->refreshTimer_->start(guard->retry_.delayMs());
                }, Qt::QueuedConnection);
        });
    } catch (const std::exception& exception) {
        snapshotPending_ = false;
        snapshotHealthy_ = false;
        retry_.failure();
        status_->setText(QString::fromUtf8(exception.what()));
    }
}
void MainWindow::attachEngine(std::unique_ptr<EngineSession::Engine> engine, const QString& label) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    ++sessionGeneration_;
    previewTimer_->stop();
    refreshTimer_->stop();
    session_ = std::make_unique<EngineSession>(std::move(engine));
    snapshotHealthy_ = false;
    retry_.success();
    clearVisualContext();
    scenarios_->setVisible(false);
    retryButton_->setVisible(true);
    banner_->setText("Sesión real v2 · " + label);
    start_->setText("Inicio continuo deshabilitado");
    start_->setToolTip("El avance disponible se ejecuta con Siguiente paso.");
    step_->setText("Siguiente paso");
    pause_->setText("Pausar");
    reset_->setText("Reiniciar");
    add_->setText("Agregar proceso");
    const int generation = sessionGeneration_;
    QPointer<MainWindow> guard(this);
    session_->prepararPaso([guard, generation](api::Resultado resultado,
                                              EngineSession::Error error) {
        if (!guard) return;
        QMetaObject::invokeMethod(guard.data(), [guard, generation, resultado, error] {
            if (!guard || generation != guard->sessionGeneration_) return;
            if (error) {
                guard->finishAction(api::CodigoResultado::ESTADO_INVALIDO, error, generation);
            } else {
                guard->report(resultado.codigo);
                guard->refreshEngine();
            }
        }, Qt::QueuedConnection);
    });
}
void MainWindow::addProcess() {
    if (session_ && !snapshotHealthy_) return;
    NewProcessDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;
    if (session_) {
        const api::AltaProceso proceso = dialog.input();
        action([this, proceso](auto callback) {
            session_->agregar(proceso, std::move(callback));
        });
    }
    else status_->setText("Formulario válido. No se admitió ningún proceso: la vista previa no es un motor.");
}
}
