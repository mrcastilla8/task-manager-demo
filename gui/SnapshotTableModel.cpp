#include "gui/SnapshotTableModel.h"
#include "gui/SnapshotComparison.h"
#include "gui/WidgetSupport.h"
#include <QStringList>
namespace gui {
SnapshotTableModel::SnapshotTableModel(Kind kind, QObject* parent) : QAbstractTableModel(parent), kind_(kind) {}
int SnapshotTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(kind_ == Kind::Processes ? processes_.size() : events_.size());
}
int SnapshotTableModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : kind_ == Kind::Processes ? 11 : 5;
}
const api::Proceso* SnapshotTableModel::process(int row) const {
    return row >= 0 && static_cast<std::size_t>(row) < processes_.size() ? &processes_[row] : nullptr;
}
QVariant SnapshotTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= rowCount() || index.column() >= columnCount()) return {};
    if (kind_ == Kind::Processes) {
        const auto& p = processes_[index.row()];
        if (role == Qt::ForegroundRole && index.column() == 0) return pidColor(p.pid).darker(180);
        if (role != Qt::DisplayRole) return {};
        switch (index.column()) {
        case 0: return p.pid;
        case 1: return QString::fromStdString(p.nombre);
        case 2: return number(p.tamanioBytes);
        case 3: return stateName(p.estado);
        case 4: return priority(p.prioridad);
        case 5: return number(api::cuotaVigenteMs(p));
        case 6: return number(p.tiempoCpuTotalMs);
        case 7: return number(p.tiempoCpuRestanteMs);
        case 8: return number(p.tiempoCpuConsumidoMs);
        case 9: return number(p.tiempoEsperaMs);
        case 10: return QString::number(static_cast<qulonglong>(p.ticketReady));
        }
    } else if (role == Qt::DisplayRole) {
        const auto& e = events_[index.row()];
        switch (index.column()) {
        case 0: return number(e.tiempoSimuladoMs);
        case 1: return eventName(e.tipo);
        case 2: return e.pid > 0 ? QString::number(e.pid) : QString("—");
        case 3: return number(e.duracionMs);
        case 4: return QString::fromStdString(e.detalle);
        }
    }
    return {};
}
QVariant SnapshotTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    static const QStringList processHeaders = {"PID", "Nombre", "Bytes", "Estado", "Prioridad", "Cuota vigente (ms)",
        "CPU total (ms)", "Restante (ms)", "Consumida (ms)", "Espera (ms)", "Ticket READY"};
    static const QStringList eventHeaders = {"Tiempo simulado (ms)", "Evento", "PID", "Duración (ms)", "Detalle"};
    return (kind_ == Kind::Processes ? processHeaders : eventHeaders).value(section);
}
template<class T> bool SnapshotTableModel::updateRows(std::vector<T>& old, const std::vector<T>& incoming) {
    bool changed = false;
    const auto overlap = std::min(old.size(), incoming.size());
    // Agrupa las filas cambiadas; una publicación idéntica no emite señales.
    std::size_t first = overlap;
    for (std::size_t i = 0; i < overlap; ++i) {
        if (!same(old[i], incoming[i])) {
            old[i] = incoming[i];
            first = std::min(first, i);
        }
    }
    if (first != overlap) {
        emit dataChanged(index(static_cast<int>(first), 0), index(static_cast<int>(overlap - 1), columnCount() - 1));
        changed = true;
    }
    if (incoming.size() > old.size()) {
        const auto count = old.size();
        beginInsertRows({}, static_cast<int>(count), static_cast<int>(incoming.size() - 1));
        old.insert(old.end(), incoming.begin() + count, incoming.end());
        endInsertRows();
        changed = true;
    } else if (incoming.size() < old.size()) {
        beginRemoveRows({}, static_cast<int>(incoming.size()), static_cast<int>(old.size() - 1));
        old.resize(incoming.size());
        endRemoveRows();
        changed = true;
    }
    if (changed) ++revision_;
    return changed;
}
bool SnapshotTableModel::present(const api::EstadoSistema& state) {
    return kind_ == Kind::Processes ? updateRows(processes_, state.procesos) : updateRows(events_, state.eventos);
}
}
