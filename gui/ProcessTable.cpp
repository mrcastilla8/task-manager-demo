#include "gui/ProcessTable.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QSignalBlocker>
namespace gui {
ProcessTable::ProcessTable(QWidget* parent) : QTableView(parent) {
    setObjectName("processTable");
    model_ = new SnapshotTableModel(SnapshotTableModel::Kind::Processes, this);
    setModel(model_);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    verticalHeader()->hide();
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    horizontalHeader()->setDefaultSectionSize(115);
}
void ProcessTable::present(const api::EstadoSistema& state) {
    const int selected = selectedPid();
    const int vertical = verticalScrollBar()->value(), horizontal = horizontalScrollBar()->value();
    const QSignalBlocker blocker(selectionModel());
    if (!model_->present(state)) return;
    clearSelection();
    setCurrentIndex({});
    if (selected > 0) {
        for (int row = 0; row < model_->rowCount(); ++row) {
            if (model_->process(row)->pid == selected) { selectRow(row); break; }
        }
    }
    updateGeometries();
    verticalScrollBar()->setValue(vertical);
    horizontalScrollBar()->setValue(horizontal);
}
int ProcessTable::selectedPid() const {
    const auto rows = selectionModel()->selectedRows();
    const auto* p = rows.isEmpty() ? nullptr : model_->process(rows.front().row());
    return p ? p->pid : -1;
}
bool ProcessTable::selectedIsActive() const {
    const auto rows = selectionModel()->selectedRows();
    const auto* p = rows.isEmpty() ? nullptr : model_->process(rows.front().row());
    return p && p->pid > 0 && active(p->estado);
}
}
