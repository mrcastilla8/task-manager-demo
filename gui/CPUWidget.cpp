#include "gui/CPUWidget.h"
#include "gui/WidgetSupport.h"
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <algorithm>
namespace gui {
CPUWidget::CPUWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    description_ = new QLabel(this);
    description_->setObjectName("cpuDescription");
    description_->setWordWrap(true);
    progress_ = new QProgressBar(this);
    progress_->setRange(0, 1000);
    layout->addWidget(description_);
    layout->addWidget(progress_);
}
void CPUWidget::present(const api::EstadoSistema& s) {
    const auto* p = findProcess(s, s.pidEjecutando);
    if (!p || p->estado != api::Estado::RUN) {
        description_->setText("CPU lógico libre · sin RUN publicado");
        progress_->setValue(0);
        progress_->setFormat("Sin turno activo");
        return;
    }
    description_->setText(QString("PID %1 · %2 · %3\nRestante: %4 ms · cuota fijada: %5 ms\nCPU del turno: %6 ms")
        .arg(p->pid).arg(QString::fromStdString(p->nombre), priority(p->prioridad))
        .arg(number(p->tiempoCpuRestanteMs), number(s.cuotaTurnoFijadaMs), number(s.cpuConsumidaTurnoMs)));
    const double fraction = s.cuotaTurnoFijadaMs > 0 ?
        static_cast<double>(s.cpuConsumidaTurnoMs) / s.cuotaTurnoFijadaMs : 0;
    progress_->setValue(static_cast<int>(std::clamp(fraction, 0.0, 1.0) * 1000));
    progress_->setFormat("%p% de cuota fijada");
}
}
