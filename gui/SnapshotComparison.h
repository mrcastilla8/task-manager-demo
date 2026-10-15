#pragma once
#include "gui/Presentation.h"
#include <algorithm>
#include <tuple>
namespace gui {
inline bool same(const api::OperacionIO& a, const api::OperacionIO& b) {
    return std::tie(a.habilitada, a.despuesDeCpuMs, a.duracionEsperaMs) ==
           std::tie(b.habilitada, b.despuesDeCpuMs, b.duracionEsperaMs);
}
inline bool same(const api::Proceso& a, const api::Proceso& b) {
    return std::tie(a.pid, a.nombre, a.tamanioBytes, a.prioridad, a.tiempoCpuTotalMs,
        a.tiempoCpuRestanteMs, a.tiempoCpuConsumidoMs, a.tiempoEsperaMs, a.ticketReady, a.estado) ==
        std::tie(b.pid, b.nombre, b.tamanioBytes, b.prioridad, b.tiempoCpuTotalMs,
        b.tiempoCpuRestanteMs, b.tiempoCpuConsumidoMs, b.tiempoEsperaMs, b.ticketReady, b.estado) &&
        same(a.operacionIO, b.operacionIO);
}
inline bool same(const api::Evento& a, const api::Evento& b) {
    return std::tie(a.tiempoSimuladoMs, a.tipo, a.pid, a.duracionMs, a.detalle) ==
           std::tie(b.tiempoSimuladoMs, b.tipo, b.pid, b.duracionMs, b.detalle);
}
inline bool same(const api::BloqueTimeline& a, const api::BloqueTimeline& b) {
    return a.pid() == b.pid() && a.inicioMs() == b.inicioMs() && a.finMs() == b.finMs();
}
template<class T> bool same(const std::vector<T>& a, const std::vector<T>& b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
        [](const T& x, const T& y) { return same(x, y); });
}
inline bool sameSnapshot(const api::EstadoSistema& a, const api::EstadoSistema& b) {
    return std::tie(a.tiempoSimuladoMs, a.estadoEjecucion, a.colaReady, a.pidEjecutando,
        a.cuotaTurnoFijadaMs, a.cpuConsumidaTurnoMs, a.memoriaUsadaBytes, a.memoriaTotalBytes) ==
        std::tie(b.tiempoSimuladoMs, b.estadoEjecucion, b.colaReady, b.pidEjecutando,
        b.cuotaTurnoFijadaMs, b.cpuConsumidaTurnoMs, b.memoriaUsadaBytes, b.memoriaTotalBytes) &&
        same(a.procesos, b.procesos) && same(a.eventos, b.eventos) && same(a.timeline, b.timeline);
}
// Política local de consulta UI, ajena al reloj y a las reglas del motor.
class SnapshotRetry {
public:
    void success() { failures_ = 0; }
    void failure() { failures_ = std::min(failures_ + 1, 6); }
    bool healthy() const { return failures_ == 0; }
    int delayMs() const { return std::min(150 * (1 << failures_), 5000); }
private:
    int failures_{0};
};
}
