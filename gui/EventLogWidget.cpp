#include "gui/EventLogWidget.h"
#include <QHeaderView>
#include <QScrollBar>
namespace gui {
EventLogWidget::EventLogWidget(QWidget* parent) : QTableView(parent) {
    setObjectName("eventLog");
    model_ = new SnapshotTableModel(SnapshotTableModel::Kind::Events, this);
    setModel(model_);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    verticalHeader()->hide();
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    horizontalHeader()->setDefaultSectionSize(160);
    setColumnWidth(1, 190);
    setColumnWidth(4, 400);
}
void EventLogWidget::present(const api::EstadoSistema& s) {
    const int vertical = verticalScrollBar()->value(), horizontal = horizontalScrollBar()->value();
    const bool atEnd = vertical == verticalScrollBar()->maximum();
    if (!model_->present(s)) return;
    updateGeometries();
    if (atEnd) scrollToBottom(); else verticalScrollBar()->setValue(vertical);
    horizontalScrollBar()->setValue(horizontal);
}
}
