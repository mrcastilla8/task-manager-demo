#pragma once

#include "contracts/EstadoSistema.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

// Devuelve el numero de invariantes violadas por un snapshot. No usa EXPECT_*
// porque se invoca desde varios hilos y los contadores de TestSupport no son
// atomicos.
inline int invariantesVioladas(const EstadoSistema& s) {
    int violaciones = 0;

    const auto enRun = std::count_if(
        s.procesos.begin(), s.procesos.end(),
        [](const Proceso& p) { return p.estado == Estado::RUN; });
    if (enRun > 1) ++violaciones;  // un unico CPU logico
    if (enRun == 0 && s.pidEjecutando != -1) ++violaciones;
    if (enRun == 1) {
        const auto it = std::find_if(
            s.procesos.begin(), s.procesos.end(),
            [](const Proceso& p) { return p.estado == Estado::RUN; });
        if (it->pid != s.pidEjecutando) ++violaciones;
    }

    std::int64_t memoriaVivos = 0;
    for (const auto& p : s.procesos) {
        if (p.estado != Estado::FINISH && p.estado != Estado::CANCEL) {
            memoriaVivos += p.tamanioBytes;
        }
    }
    if (memoriaVivos != s.memoriaUsada) ++violaciones;
    if (s.memoriaUsada > s.memoriaTotal) ++violaciones;

    std::int64_t finAnterior = 0;
    for (const auto& bloque : s.timeline) {
        if (!bloqueTimelineValido(bloque)) ++violaciones;
        if (bloque.inicio < finAnterior) ++violaciones;  // solape o desorden
        if (bloque.fin > s.tiempoSimulado) ++violaciones;
        finAnterior = bloque.fin;
    }

    std::int64_t tiempoEvento = 0;
    for (const auto& evento : s.eventos) {
        if (evento.tiempoSimulado < tiempoEvento) ++violaciones;
        tiempoEvento = evento.tiempoSimulado;
    }

    // La cola solo contiene procesos READY, sin repetidos y completa.
    const auto listos = std::count_if(
        s.procesos.begin(), s.procesos.end(),
        [](const Proceso& p) { return p.estado == Estado::READY; });
    if (static_cast<std::ptrdiff_t>(s.colaPrioridad.size()) != listos) {
        ++violaciones;
    }
    for (int pid : s.colaPrioridad) {
        const auto it = std::find_if(
            s.procesos.begin(), s.procesos.end(),
            [pid](const Proceso& p) { return p.pid == pid; });
        if (it == s.procesos.end() || it->estado != Estado::READY) {
            ++violaciones;
        }
    }

    return violaciones;
}
