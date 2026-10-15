#include "gui/EventLogWidget.h"
#include "gui/WidgetSupport.h"
#include <QScrollBar>
namespace gui {
EventLogWidget::EventLogWidget(QWidget* parent) : QTableWidget(parent) {
    setObjectName("eventLog");
    setupTable(this, {"Tiempo simulado (ms)", "Evento", "PID", "Duración (ms)", "Detalle"});
}
void EventLogWidget::present(const api::EstadoSistema& s) {
    const int oldPosition = verticalScrollBar()->value();
    const bool atEnd = oldPosition == verticalScrollBar()->maximum();
    setRowCount(static_cast<int>(s.eventos.size()));
    int row = 0;
    // Conserva orden recibido, sin inferir precedencia para timestamps iguales.
    for (const auto& event : s.eventos) {
        cell(this, row, 0, number(event.tiempoSimuladoMs));
        cell(this, row, 1, eventName(event.tipo));
        cell(this, row, 2, event.pid > 0 ? QString::number(event.pid) : "—");
        cell(this, row, 3, number(event.duracionMs));
        cell(this, row, 4, QString::fromStdString(event.detalle));
        ++row;
    }
    if (atEnd) scrollToBottom(); else verticalScrollBar()->setValue(oldPosition);
}
}
