#pragma once
#include "gui/Presentation.h"
#include <QListWidget>
namespace gui {
class QueueWidget : public QListWidget {
public:
    explicit QueueWidget(QWidget* parent = nullptr);
    void present(const api::EstadoSistema& state);
};
}
