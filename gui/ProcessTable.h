#pragma once
#include "gui/Presentation.h"
#include "gui/SnapshotTableModel.h"
#include <QTableView>
namespace gui {
class ProcessTable : public QTableView {
public:
    explicit ProcessTable(QWidget* parent = nullptr);
    void present(const api::EstadoSistema& state);
    int selectedPid() const;
    bool selectedIsActive() const;
    SnapshotTableModel* presentationModel() const { return model_; }
private:
    SnapshotTableModel* model_;
};
}
