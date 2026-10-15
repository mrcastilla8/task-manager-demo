#pragma once
#include "gui/Presentation.h"
#include <QTableWidget>
namespace gui {
class EventLogWidget : public QTableWidget {
public:
    explicit EventLogWidget(QWidget* parent = nullptr);
    void present(const api::EstadoSistema& state);
};
}
