#include "gui/QueueWidget.h"
#include "gui/WidgetSupport.h"
namespace gui {
QueueWidget::QueueWidget(QWidget* parent) : QListWidget(parent) { setObjectName("readyQueue"); }
void QueueWidget::present(const api::EstadoSistema& state) {
    clear();
    for (int pid : state.colaReady) {
        const auto* p = findProcess(state, pid);
        // No reparar ni reordenar una publicación incoherente del motor.
        if (!p || p->estado != api::Estado::READY) {
            addItem(QString("Snapshot incoherente: PID %1 no está READY").arg(pid));
            continue;
        }
        addItem(QString("PID %1 · %2 · %3 · ticket %4")
            .arg(pid).arg(QString::fromStdString(p->nombre), priority(p->prioridad))
            .arg(static_cast<qulonglong>(p->ticketReady)));
    }
    if (state.colaReady.empty()) addItem("Sin procesos READY");
}
}
