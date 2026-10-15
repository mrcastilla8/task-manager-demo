#pragma once
#include "gui/Presentation.h"
#include "gui/SnapshotTableModel.h"
#include <QTableView>
namespace gui {
class EventLogWidget : public QTableView {
public:
    explicit EventLogWidget(QWidget* parent = nullptr);
    void present(const api::EstadoSistema& state);
    SnapshotTableModel* presentationModel() const { return model_; }
private:
    SnapshotTableModel* model_;
};
}
