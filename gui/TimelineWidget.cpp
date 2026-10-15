#include "gui/TimelineWidget.h"
#include "gui/WidgetSupport.h"
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <algorithm>
namespace gui {
TimelineWidget::TimelineWidget(QWidget* parent) : QGraphicsView(parent) {
    setObjectName("timeline");
    setScene(new QGraphicsScene(this));
    setMinimumHeight(150);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setDragMode(QGraphicsView::ScrollHandDrag);
}
void TimelineWidget::present(const api::EstadoSistema& state) { state_ = state; rebuild(); }
void TimelineWidget::setZoom(int value) { scale_ = std::clamp(value, 1, 80); rebuild(); }
void TimelineWidget::rebuild() {
    scene()->clear();
    try {
        const auto spans = timelineSpans(state_);
        const double extent = static_cast<double>(state_.tiempoSimuladoMs) * scale_;
        scene()->setSceneRect(0, 0, std::max(600.0, extent + 80), 115);
        for (const auto& span : spans) {
            const double x = static_cast<double>(span.start) * scale_;
            const double size = static_cast<double>(span.end - span.start) * scale_;
            auto* rect = scene()->addRect(x, 30, size, 40, QPen(Qt::white),
                span.pid > 0 ? QBrush(pidColor(span.pid)) : QBrush(QColor("#dce3ec"), Qt::BDiagPattern));
            const QString label = span.pid > 0 ? QString("PID %1").arg(span.pid) : "idle";
            rect->setToolTip(QString("%1 [%2,%3) ms").arg(label, number(span.start), number(span.end)));
            if (size > 45) scene()->addSimpleText(label)->setPos(x + 4, 40);
            scene()->addSimpleText(number(span.start))->setPos(x, 8);
        }
        scene()->addSimpleText(number(state_.tiempoSimuladoMs) + " ms")->setPos(extent, 78);
    } catch (const std::exception& e) {
        scene()->addSimpleText(QString::fromUtf8(e.what()));
    }
}
}
