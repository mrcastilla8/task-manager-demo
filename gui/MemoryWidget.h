#pragma once
#include "gui/Presentation.h"
#include <QWidget>
namespace gui {
class MemoryWidget : public QWidget {
public:
    explicit MemoryWidget(QWidget* parent = nullptr);
    void present(const api::EstadoSistema& state);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    api::EstadoSistema state_;
};
}
