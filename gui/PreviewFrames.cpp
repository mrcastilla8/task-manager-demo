#include "gui/PreviewFrames.h"
namespace gui {
namespace api = ConcurrentSimulator::v2;
std::vector<PreviewFrame> previewFrames() {
    api::EstadoSistema s;
    s.estadoEjecucion = api::EstadoEjecucion::PAUSADO;
    s.memoriaTotalBytes = 1024;
    s.memoriaUsadaBytes = 420;
    // Clases y orden ya publicados por la fixture, nunca calculados en la vista.
    s.procesos = {
        {1, "Editor", 60, api::Prioridad::P1, 16, 16, 0, 0, 1, api::Estado::READY, {true, 8, 10}},
        {2, "Compilador", 300, api::Prioridad::P3, 4, 4, 0, 0, 2, api::Estado::READY, {}},
        {3, "Terminal", 60, api::Prioridad::P1, 12, 12, 0, 0, 3, api::Estado::READY, {}}
    };
    s.colaReady = {2, 1, 3};
    s.eventos = {{0, api::TipoEvento::PROCESO_ADMITIDO, 1, 0, "Fixture: Editor"},
                 {0, api::TipoEvento::PROCESO_ADMITIDO, 2, 0, "Fixture: Compilador"},
                 {0, api::TipoEvento::PROCESO_ADMITIDO, 3, 0, "Fixture: Terminal"}};
    std::vector<PreviewFrame> frames{{"READY: prioridades y empate por ticket", s}};

    s.pidEjecutando = 2;
    s.colaReady = {1, 3};
    s.procesos[1].estado = api::Estado::RUN;
    s.cuotaTurnoFijadaMs = 30;
    s.eventos.push_back({0, api::TipoEvento::READY_A_RUN, 2, 0, "RUN visible por snapshot explícito"});
    frames.push_back({"RUN: cuota fijada de 30 ms", s});

    s.tiempoSimuladoMs = 4;
    s.timeline.emplace_back(2, 0, 4);
    s.procesos[1].estado = api::Estado::FINISH;
    s.procesos[1].tiempoCpuRestanteMs = 0;
    s.procesos[1].tiempoCpuConsumidoMs = 4;
    s.memoriaUsadaBytes = 120;
    s.pidEjecutando = -1;
    s.cuotaTurnoFijadaMs = 0;
    s.procesos[0].prioridad = s.procesos[2].prioridad = api::Prioridad::P3;
    s.eventos.push_back({4, api::TipoEvento::RUN_A_FINISH, 2, 0, "Memoria liberada en fixture"});
    frames.push_back({"FINISH: historial y memoria liberada", s});

    s.tiempoSimuladoMs = 7;
    s.procesos[0].estado = api::Estado::RUN;
    s.procesos[0].tiempoCpuRestanteMs = 13;
    s.procesos[0].tiempoCpuConsumidoMs = 3;
    s.pidEjecutando = 1;
    s.colaReady = {3};
    s.cuotaTurnoFijadaMs = 30;
    s.cpuConsumidaTurnoMs = 3;
    s.timeline.emplace_back(1, 4, 7);
    s.eventos.push_back({4, api::TipoEvento::READY_A_RUN, 1, 0, "Turno Editor"});
    frames.push_back({"RUN parcial: 3 ms consumidos de cuota 30 ms", s});

    s.tiempoSimuladoMs = 12;
    s.timeline.back() = api::BloqueTimeline(1, 4, 12);
    s.procesos[0].estado = api::Estado::WAIT;
    s.procesos[0].tiempoCpuRestanteMs = 8;
    s.procesos[0].tiempoCpuConsumidoMs = 8;
    s.procesos[2].estado = api::Estado::CANCEL;
    s.memoriaUsadaBytes = 60;
    s.pidEjecutando = -1;
    s.colaReady.clear();
    s.cuotaTurnoFijadaMs = s.cpuConsumidaTurnoMs = 0;
    s.eventos.push_back({12, api::TipoEvento::RUN_A_WAIT, 1, 0, "E/S; conserva RAM"});
    s.eventos.push_back({12, api::TipoEvento::PROCESO_CANCELADO, 3, 0, "Terminal cancelada en fixture"});
    frames.push_back({"WAIT y CANCEL: sin procesos READY", s});

    s.tiempoSimuladoMs = 22;
    s.procesos[0].estado = api::Estado::READY;
    s.procesos[0].ticketReady = 4;
    s.colaReady = {1};
    s.eventos.push_back({22, api::TipoEvento::WAIT_A_READY, 1, 0, "Retorno sin despacho en este paso"});
    frames.push_back({"Idle [12,22): despertar con ticket nuevo", s});

    s.tiempoSimuladoMs = 30;
    s.timeline.emplace_back(1, 22, 30);
    s.procesos[0].estado = api::Estado::FINISH;
    s.procesos[0].tiempoCpuRestanteMs = 0;
    s.procesos[0].tiempoCpuConsumidoMs = 16;
    s.colaReady.clear();
    s.memoriaUsadaBytes = 0;
    s.estadoEjecucion = api::EstadoEjecucion::FINALIZADO;
    s.eventos.push_back({30, api::TipoEvento::RUN_A_FINISH, 1, 0, "Fin Editor"});
    s.eventos.push_back({30, api::TipoEvento::SISTEMA_FINALIZADO, -1, 0, "Fin de fixture, no ejecución real"});
    frames.push_back({"Sesión terminada: historial completo", s});

    api::EstadoSistema rotation;
    rotation.memoriaTotalBytes = 1024;
    rotation.memoriaUsadaBytes = 256;
    rotation.estadoEjecucion = api::EstadoEjecucion::PAUSADO;
    rotation.tiempoSimuladoMs = 30;
    rotation.procesos = {
        {11, "Lote A", 128, api::Prioridad::P3, 45, 15, 30, 0, 3, api::Estado::READY, {}},
        {12, "Lote B", 128, api::Prioridad::P3, 40, 40, 0, 0, 2, api::Estado::READY, {}}
    };
    rotation.colaReady = {12, 11};
    rotation.timeline = {{11, 0, 30}};
    rotation.eventos = {
        {0, api::TipoEvento::PROCESO_ADMITIDO, 11, 0, "Fixture independiente de rotación"},
        {0, api::TipoEvento::PROCESO_ADMITIDO, 12, 0, "Segundo ticket"},
        {0, api::TipoEvento::READY_A_RUN, 11, 0, "Cuota fijada 30 ms"},
        {30, api::TipoEvento::CUOTA_AGOTADA, 11, 0, "CPU pendiente: 15 ms"},
        {30, api::TipoEvento::RUN_A_READY, 11, 0, "Reingreso con ticket 3; PID 12 delante"}
    };
    frames.push_back({"Fixture independiente: cuota agotada y ticket nuevo", rotation});

    api::EstadoSistema gaps;
    gaps.memoriaTotalBytes = 1024;
    gaps.tiempoSimuladoMs = 27;
    gaps.timeline = {{1, 2, 8}, {2, 18, 22}};
    frames.push_back({"Fixture independiente: idle antes, entre y después", gaps});
    return frames;
}
}
