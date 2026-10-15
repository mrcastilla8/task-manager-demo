#include "gui/TimelineWidget.h"
#include "gui/SnapshotComparison.h"
#include "gui/WidgetSupport.h"
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>
#include <algorithm>
#include <climits>
namespace gui {
// Un item para el historial completo; búsqueda binaria de los spans expuestos.
class TimelineCanvas : public QGraphicsItem {
public:
    std::vector<TimelineSpan> spans;
    std::int64_t time{0};
    int scale{12};
    TimelineCanvas() { setFlag(ItemUsesExtendedStyleOption); setAcceptHoverEvents(true); }
    QRectF boundingRect() const override { return {0, 0, std::max(600.0, static_cast<double>(time) * scale + 80), 115}; }
    void geometry(std::int64_t t, int zoom) {
        prepareGeometryChange();
        time = t;
        scale = zoom;
        update();
    }
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) override {
        const auto visible = option->exposedRect;
        const double left = visible.left() / scale, right = visible.right() / scale;
        auto it = std::lower_bound(spans.begin(), spans.end(), left,
            [](const TimelineSpan& s, double t) { return static_cast<double>(s.end) < t; });
        painter->setPen(Qt::black);
        for (; it != spans.end() && static_cast<double>(it->start) <= right; ++it) {
            const double x = static_cast<double>(it->start) * scale;
            const double size = static_cast<double>(it->end - it->start) * scale;
            const QRectF rect(x, 30, size, 40);
            painter->fillRect(rect, it->pid > 0 ? QBrush(pidColor(it->pid)) : QBrush(QColor("#dce3ec"), Qt::BDiagPattern));
            const QString label = it->pid > 0 ? QString("PID %1").arg(it->pid) : "idle";
            if (size > 45) painter->drawText(rect.adjusted(4, 0, 0, 0), Qt::AlignVCenter, label);
            painter->drawText(QPointF(x, 22), number(it->start));
        }
        painter->drawText(QPointF(static_cast<double>(time) * scale, 90), number(time) + " ms");
    }
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override {
        const double t = event->pos().x() / scale;
        const auto it = std::lower_bound(spans.begin(), spans.end(), t,
            [](const TimelineSpan& s, double point) { return static_cast<double>(s.end) <= point; });
        if (it != spans.end() && static_cast<double>(it->start) <= t)
            setToolTip(QString("%1 [%2,%3) ms").arg(it->pid > 0 ? QString("PID %1").arg(it->pid) : "idle",
                number(it->start), number(it->end)));
        else setToolTip({});
    }
};
TimelineWidget::TimelineWidget(QWidget* parent) : QGraphicsView(parent) {
    setObjectName("timeline");
    setScene(new QGraphicsScene(this));
    canvas_ = new TimelineCanvas;
    scene()->addItem(canvas_);
    setMinimumHeight(150);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setDragMode(QGraphicsView::ScrollHandDrag);
}
void TimelineWidget::present(const api::EstadoSistema& state) {
    if (time_ == state.tiempoSimuladoMs && same(blocks_, state.timeline)) return;
    auto spans = timelineSpans(state); // Validar antes de cambiar el cache.
    const int x = horizontalScrollBar()->value(), y = verticalScrollBar()->value();
    blocks_ = state.timeline;
    time_ = state.tiempoSimuladoMs;
    canvas_->spans = std::move(spans);
    canvas_->geometry(time_, scale_);
    scene()->setSceneRect(canvas_->boundingRect());
    horizontalScrollBar()->setValue(x);
    verticalScrollBar()->setValue(y);
    ++revision_;
}
void TimelineWidget::setZoom(int value) {
    value = std::clamp(value, 1, 80);
    if (value == scale_) return;
    const double positionMs = static_cast<double>(horizontalScrollBar()->value()) / scale_;
    scale_ = value;
    canvas_->geometry(std::max<std::int64_t>(time_, 0), scale_);
    scene()->setSceneRect(canvas_->boundingRect());
    horizontalScrollBar()->setValue(static_cast<int>(std::min(positionMs * scale_, static_cast<double>(INT_MAX))));
}
std::size_t TimelineWidget::spanCount() const { return canvas_->spans.size(); }
void TimelineWidget::resetContext() {
    horizontalScrollBar()->setValue(0);
    verticalScrollBar()->setValue(0);
}
}
