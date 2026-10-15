#pragma once
#include "gui/Presentation.h"
#include <QGraphicsView>
namespace gui {
class TimelineWidget : public QGraphicsView {
public:
    explicit TimelineWidget(QWidget* parent = nullptr);
    void present(const api::EstadoSistema& state);
    void setZoom(int pixelsPerMs);
private:
    void rebuild();
    api::EstadoSistema state_;
    int scale_{12};
};
}
