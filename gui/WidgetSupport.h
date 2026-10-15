#pragma once
#include "gui/Presentation.h"
#include <QColor>
#include <QString>
#include <QTableWidget>
#include <QHeaderView>
namespace gui {
inline QString number(std::int64_t value) { return QString::number(static_cast<qlonglong>(value)); }
inline QColor pidColor(int pid) { return QColor::fromHsv((pid * 47LL % 360 + 360) % 360, 135, 215); }
inline QString priority(api::Prioridad value) { return "P" + QString::number(static_cast<int>(value)); }
inline void setupTable(QTableWidget* table, const QStringList& labels) {
    table->setColumnCount(labels.size());
    table->setHorizontalHeaderLabels(labels);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setSortingEnabled(false);
    table->verticalHeader()->hide();
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
}
inline void cell(QTableWidget* table, int row, int column, const QString& text) {
    table->setItem(row, column, new QTableWidgetItem(text));
}
}
