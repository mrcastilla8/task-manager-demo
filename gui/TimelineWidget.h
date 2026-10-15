#pragma once
#include "gui/Presentation.h"
#include <QGraphicsView>
namespace gui {
class TimelineCanvas;
class TimelineWidget : public QGraphicsView {
public:
    explicit TimelineWidget(QWidget* parent = nullptr);
    void present(const api::EstadoSistema& state);
    void setZoom(int pixelsPerMs);
    int zoom() const { return scale_; }
    std::size_t revision() const { return revision_; }
    std::size_t spanCount() const;
    void resetContext();
private:
    TimelineCanvas* canvas_;
    std::vector<api::BloqueTimeline> blocks_;
    std::int64_t time_{-1};
    int scale_{12};
    std::size_t revision_{0};
};
}
