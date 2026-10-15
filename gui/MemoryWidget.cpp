#include "gui/MemoryWidget.h"
#include "gui/WidgetSupport.h"
#include "gui/SnapshotComparison.h"
#include <QPainter>
#include <QStringList>
namespace gui {
MemoryWidget::MemoryWidget(QWidget* parent) : QWidget(parent) { setMinimumHeight(145); }
void MemoryWidget::present(const api::EstadoSistema& state) {
    if (state_.memoriaTotalBytes == state.memoriaTotalBytes && state_.memoriaUsadaBytes == state.memoriaUsadaBytes &&
        same(state_.procesos, state.procesos)) return;
    state_.memoriaTotalBytes = state.memoriaTotalBytes;
    state_.memoriaUsadaBytes = state.memoriaUsadaBytes;
    state_.procesos = state.procesos; // RAM no necesita copiar eventos ni timeline.
    QStringList occupants;
    for (const auto& p : state.procesos)
        if (active(p.estado)) occupants << QString("PID %1: %2 bytes").arg(p.pid).arg(number(p.tamanioBytes));
    setToolTip(occupants.join('\n'));
    update();
}
void MemoryWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setPen(palette().text().color());
    painter.drawText(8, 22, QString("%1 / %2 bytes · mapa conceptual")
        .arg(number(state_.memoriaUsadaBytes), number(state_.memoriaTotalBytes)));
    const QRectF bar(8, 38, width() - 16, 42);
    painter.fillRect(bar, QColor("#dce3ec"));
    if (state_.memoriaTotalBytes <= 0) {
        painter.drawText(bar, Qt::AlignCenter, "Sin capacidad publicada");
        return;
    }
    painter.save();
    painter.setClipRect(bar);
    double x = bar.left();
    for (const auto& process : state_.procesos) {
        if (!active(process.estado)) continue;
        const double size = bar.width() * static_cast<double>(process.tamanioBytes) / state_.memoriaTotalBytes;
        const QRectF block(x, bar.top(), size, bar.height());
        painter.fillRect(block, pidColor(process.pid));
        painter.setPen(Qt::black);
        if (size > 40) painter.drawText(block, Qt::AlignCenter, QString("%1").arg(process.pid));
        x += size;
    }
    painter.restore();
    painter.setPen(palette().text().color());
    painter.drawText(8, 103, QString("Libre: %1 bytes · WAIT conserva RAM")
        .arg(number(state_.memoriaTotalBytes - state_.memoriaUsadaBytes)));
    painter.drawText(8, 125, "Color = PID · FINISH/CANCEL fuera de memoria");
}
}
