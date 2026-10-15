#pragma once
#include "gui/Presentation.h"
#include <QAbstractTableModel>
namespace gui {
// Qt solicita solo las celdas visibles; no se crean QTableWidgetItem por entrada.
class SnapshotTableModel : public QAbstractTableModel {
public:
    enum class Kind { Processes, Events };
    explicit SnapshotTableModel(Kind kind, QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool present(const api::EstadoSistema& state);
    const api::Proceso* process(int row) const;
    std::size_t revision() const { return revision_; }
private:
    template<class T> bool updateRows(std::vector<T>& old, const std::vector<T>& incoming);
    Kind kind_;
    std::vector<api::Proceso> processes_;
    std::vector<api::Evento> events_;
    std::size_t revision_{0};
};
}
