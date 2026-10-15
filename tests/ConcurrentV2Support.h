#pragma once

#include "concurrent/ConcurrentEngineV2.h"
#include "scheduler/SchedulerV2.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// Utilidades compartidas por las pruebas del motor concurrente v2.
namespace ConcurrentSimulator::v2::testing {

inline AltaProceso alta(int pid, std::int64_t tamanioBytes,
                        std::int64_t cpuMs, OperacionIO io = {}) {
    AltaProceso resultado;
    resultado.pid = pid;
    resultado.nombre = "P" + std::to_string(pid);
    resultado.tamanioBytes = tamanioBytes;
    resultado.tiempoCpuTotalMs = cpuMs;
    resultado.operacionIO = io;
    return resultado;
}

inline const Proceso* buscar(const EstadoSistema& estado, int pid) {
    const auto it = std::find_if(
        estado.procesos.begin(), estado.procesos.end(),
        [pid](const Proceso& proceso) { return proceso.pid == pid; });
    return it == estado.procesos.end() ? nullptr : &*it;
}

inline std::vector<TipoEvento> tiposDe(const EstadoSistema& estado, int pid) {
    std::vector<TipoEvento> tipos;
    for (const Evento& evento : estado.eventos) {
        if (evento.pid == pid) {
            tipos.push_back(evento.tipo);
        }
    }
    return tipos;
}

inline std::ptrdiff_t contarEventos(const EstadoSistema& estado,
                                   TipoEvento tipo, int pid = -2) {
    return std::count_if(estado.eventos.begin(), estado.eventos.end(),
                         [tipo, pid](const Evento& evento) {
                             return evento.tipo == tipo &&
                                    (pid == -2 || evento.pid == pid);
                         });
}

// Espera activa con yield (no sleep): en Windows sleep_for(1ms) puede dormir
// ~15 ms y se perderian ventanas cortas. Siempre acotada.
inline bool esperarHasta(const std::function<bool()>& condicion,
                         int limiteMs = 5000) {
    const auto limite = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(limiteMs);
    while (std::chrono::steady_clock::now() < limite) {
        if (condicion()) {
            return true;
        }
        std::this_thread::yield();
    }
    return condicion();
}

// Espera a que haya un proceso en RUN (pid == -2) o uno concreto.
inline bool esperarRun(const ConcurrentEngineV2& engine, int pid = -2) {
    return esperarHasta([&engine, pid]() {
        const int actual = engine.obtenerEstado().pidEjecutando;
        return pid == -2 ? actual != -1 : actual == pid;
    });
}

// Ejecuta un siguientePaso() en otro hilo con la puerta de pruebas activa y lo
// deja retenido con el proceso en RUN. terminar() libera la rafaga y une el hilo.
class PasoRetenido {
public:
    explicit PasoRetenido(ConcurrentEngineV2& engine, int pid = -2)
        : engine_(engine) {
        hilo_ = std::thread([this]() { resultado_ = engine_.siguientePaso(); });
        enRun_ = esperarRun(engine_, pid);
    }
    PasoRetenido(const PasoRetenido&) = delete;
    PasoRetenido& operator=(const PasoRetenido&) = delete;
    ~PasoRetenido() { terminar(); }

    bool enRun() const { return enRun_; }
    ResultadoPaso terminar() {
        if (hilo_.joinable()) {
            engine_.liberarRafagaPruebas();
            hilo_.join();
        }
        return resultado_;
    }

private:
    ConcurrentEngineV2& engine_;
    std::thread hilo_;
    ResultadoPaso resultado_{};
    bool enRun_{false};
};

// Compara dos snapshots campo a campo. Devuelve la descripcion de la primera
// diferencia o una cadena vacia si son identicos.
inline std::string diferencia(const EstadoSistema& a, const EstadoSistema& b) {
    const auto distinto = [](const char* campo) {
        return std::string("difiere ") + campo;
    };
    if (a.tiempoSimuladoMs != b.tiempoSimuladoMs) return distinto("tiempoSimuladoMs");
    if (a.estadoEjecucion != b.estadoEjecucion) return distinto("estadoEjecucion");
    if (a.colaReady != b.colaReady) return distinto("colaReady");
    if (a.pidEjecutando != b.pidEjecutando) return distinto("pidEjecutando");
    if (a.cuotaTurnoFijadaMs != b.cuotaTurnoFijadaMs) return distinto("cuotaTurnoFijadaMs");
    if (a.cpuConsumidaTurnoMs != b.cpuConsumidaTurnoMs) return distinto("cpuConsumidaTurnoMs");
    if (a.memoriaUsadaBytes != b.memoriaUsadaBytes) return distinto("memoriaUsadaBytes");
    if (a.memoriaTotalBytes != b.memoriaTotalBytes) return distinto("memoriaTotalBytes");
    if (a.procesos.size() != b.procesos.size()) return distinto("cantidad de procesos");
    if (a.eventos.size() != b.eventos.size()) return distinto("cantidad de eventos");
    if (a.timeline.size() != b.timeline.size()) return distinto("cantidad de bloques");
    for (std::size_t i = 0; i < a.procesos.size(); ++i) {
        const Proceso& x = a.procesos[i];
        const Proceso& y = b.procesos[i];
        if (x.pid != y.pid || x.estado != y.estado ||
            x.prioridad != y.prioridad || x.ticketReady != y.ticketReady ||
            x.tiempoCpuConsumidoMs != y.tiempoCpuConsumidoMs ||
            x.tiempoCpuRestanteMs != y.tiempoCpuRestanteMs ||
            x.tiempoEsperaMs != y.tiempoEsperaMs) {
            return "difiere el proceso " + std::to_string(x.pid);
        }
    }
    for (std::size_t i = 0; i < a.eventos.size(); ++i) {
        const Evento& x = a.eventos[i];
        const Evento& y = b.eventos[i];
        if (x.tiempoSimuladoMs != y.tiempoSimuladoMs || x.tipo != y.tipo ||
            x.pid != y.pid || x.duracionMs != y.duracionMs ||
            x.detalle != y.detalle) {
            return "difiere el evento " + std::to_string(i);
        }
    }
    for (std::size_t i = 0; i < a.timeline.size(); ++i) {
        if (a.timeline[i].pid() != b.timeline[i].pid() ||
            a.timeline[i].inicioMs() != b.timeline[i].inicioMs() ||
            a.timeline[i].finMs() != b.timeline[i].finMs()) {
            return "difiere el bloque " + std::to_string(i);
        }
    }
    return {};
}

inline bool esActivo(Estado estado) {
    return estado == Estado::READY || estado == Estado::RUN ||
           estado == Estado::WAIT;
}

// Invariantes que debe cumplir cualquier snapshot, incluso con una rafaga en
// curso. Devuelve cuantas se violan (no usa EXPECT_* para poder llamarse desde
// varios hilos).
inline int invariantesVioladas(const EstadoSistema& s) {
    int violaciones = 0;

    // Un unico CPU logico.
    const auto enRun = std::count_if(
        s.procesos.begin(), s.procesos.end(),
        [](const Proceso& p) { return p.estado == Estado::RUN; });
    if (enRun > 1) ++violaciones;
    if (enRun == 0 && s.pidEjecutando != -1) ++violaciones;
    if (enRun == 1) {
        const auto it = std::find_if(
            s.procesos.begin(), s.procesos.end(),
            [](const Proceso& p) { return p.estado == Estado::RUN; });
        if (it->pid != s.pidEjecutando) ++violaciones;
    }
    // La cuota del turno solo existe mientras hay un proceso en RUN.
    if (s.pidEjecutando == -1 &&
        (s.cuotaTurnoFijadaMs != 0 || s.cpuConsumidaTurnoMs != 0)) {
        ++violaciones;
    }
    if (s.pidEjecutando != -1 && s.cuotaTurnoFijadaMs != 10 &&
        s.cuotaTurnoFijadaMs != 20 && s.cuotaTurnoFijadaMs != 30) {
        ++violaciones;
    }

    // Memoria: solo los procesos no terminados la ocupan (WAIT incluido).
    std::int64_t memoria = 0;
    for (const Proceso& p : s.procesos) {
        if (esActivo(p.estado)) memoria += p.tamanioBytes;
        if (p.tiempoCpuConsumidoMs + p.tiempoCpuRestanteMs != p.tiempoCpuTotalMs) {
            ++violaciones;
        }
        if (p.estado == Estado::FINISH && p.tiempoCpuRestanteMs != 0) ++violaciones;
    }
    if (memoria != s.memoriaUsadaBytes) ++violaciones;
    if (s.memoriaUsadaBytes > s.memoriaTotalBytes) ++violaciones;

    // Timeline ordenado, sin solapes y dentro del reloj.
    std::int64_t finAnterior = 0;
    for (const BloqueTimeline& bloque : s.timeline) {
        if (bloque.inicioMs() < finAnterior) ++violaciones;
        if (bloque.finMs() > s.tiempoSimuladoMs) ++violaciones;
        finAnterior = bloque.finMs();
    }

    // Los eventos no retroceden en el tiempo.
    std::int64_t tiempoEvento = 0;
    for (const Evento& evento : s.eventos) {
        if (evento.tiempoSimuladoMs < tiempoEvento) ++violaciones;
        tiempoEvento = evento.tiempoSimuladoMs;
    }

    // La cola contiene exactamente los READY, sin repetidos, con ticket unico.
    const auto listos = std::count_if(
        s.procesos.begin(), s.procesos.end(),
        [](const Proceso& p) { return p.estado == Estado::READY; });
    if (static_cast<std::ptrdiff_t>(s.colaReady.size()) != listos) ++violaciones;
    std::vector<std::uint64_t> tickets;
    for (int pid : s.colaReady) {
        const Proceso* p = buscar(s, pid);
        if (p == nullptr || p->estado != Estado::READY || p->ticketReady == 0) {
            ++violaciones;
        } else {
            tickets.push_back(p->ticketReady);
        }
    }
    std::sort(tickets.begin(), tickets.end());
    if (std::adjacent_find(tickets.begin(), tickets.end()) != tickets.end()) {
        ++violaciones;
    }
    return violaciones;
}

// DP-02: las prioridades almacenadas de los procesos activos coinciden con la
// clasificacion del Scheduler oficial sobre la poblacion TL = {READY, RUN, WAIT}.
inline bool prioridadesCoherentes(const EstadoSistema& estado) {
    std::vector<Proceso> copia = estado.procesos;
    Scheduler().clasificarPrioridades(copia);
    for (std::size_t i = 0; i < copia.size(); ++i) {
        if (esActivo(copia[i].estado) &&
            copia[i].prioridad != estado.procesos[i].prioridad) {
            return false;
        }
    }
    return true;
}

}  // namespace ConcurrentSimulator::v2::testing
