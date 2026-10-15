#pragma once
#include "gui/Presentation.h"
#include <QWidget>
class QLabel;
class QProgressBar;
namespace gui {
class CPUWidget : public QWidget {
public:
    explicit CPUWidget(QWidget* parent = nullptr);
    void present(const api::EstadoSistema& state);
private:
    QLabel* description_;
    QProgressBar* progress_;
};
}
