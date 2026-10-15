#pragma once
#include "gui/Presentation.h"
#include <QTableWidget>
namespace gui {
class ProcessTable : public QTableWidget {
public:
    explicit ProcessTable(QWidget* parent = nullptr);
    void present(const api::EstadoSistema& state);
    int selectedPid() const;
};
}
