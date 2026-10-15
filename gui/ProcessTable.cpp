#include "gui/ProcessTable.h"
#include "gui/WidgetSupport.h"
namespace gui {
ProcessTable::ProcessTable(QWidget* parent) : QTableWidget(parent) {
    setObjectName("processTable");
    setupTable(this, {"PID", "Nombre", "Bytes", "Estado", "Prioridad", "Cuota vigente (ms)",
                     "CPU total (ms)", "Restante (ms)", "Consumida (ms)", "Espera (ms)", "Ticket READY"});
}
void ProcessTable::present(const api::EstadoSistema& state) {
    const int selected = selectedPid();
    setRowCount(static_cast<int>(state.procesos.size()));
    int row = 0;
    for (const auto& p : state.procesos) {
        cell(this, row, 0, QString::number(p.pid));
        cell(this, row, 1, QString::fromStdString(p.nombre));
        cell(this, row, 2, number(p.tamanioBytes));
        cell(this, row, 3, stateName(p.estado));
        cell(this, row, 4, priority(p.prioridad));
        cell(this, row, 5, number(api::cuotaVigenteMs(p)));
        cell(this, row, 6, number(p.tiempoCpuTotalMs));
        cell(this, row, 7, number(p.tiempoCpuRestanteMs));
        cell(this, row, 8, number(p.tiempoCpuConsumidoMs));
        cell(this, row, 9, number(p.tiempoEsperaMs));
        cell(this, row, 10, QString::number(static_cast<qulonglong>(p.ticketReady)));
        item(row, 0)->setForeground(pidColor(p.pid).darker(180));
        if (p.pid == selected) selectRow(row);
        ++row;
    }
}
int ProcessTable::selectedPid() const {
    const auto* value = item(currentRow(), 0);
    return value ? value->text().toInt() : -1;
}
}
