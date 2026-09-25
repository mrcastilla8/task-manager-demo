#pragma once

#include "concurrent/Synchronization.h"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

class ProcessThread {
public:
    explicit ProcessThread(int pid);
    ~ProcessThread();

    ProcessThread(const ProcessThread&) = delete;
    ProcessThread& operator=(const ProcessThread&) = delete;
    ProcessThread(ProcessThread&&) = delete;
    ProcessThread& operator=(ProcessThread&&) = delete;

    int pid() const noexcept;

    // Concede permiso exclusivo al hilo para ejecutar una rafaga.
    void concederTurno(const SolicitudTurno& solicitud);

    // Bloquea hasta que el hilo complete la rafaga asignada.
    ResultadoRafaga esperarFinTurno();

    // Solicita la detencion segura del hilo y hace join si es joinable.
    void solicitarDetencion();

    bool estaDetenido() const noexcept;

private:
    void ejecutarLoopWorker();

    int pid_;
    std::thread worker_;
    mutable std::mutex mutex_;
    std::condition_variable cvTurno_;
    std::condition_variable cvFinTurno_;

    bool permisoEjecucion_{false};
    bool turnoCompletado_{false};
    bool detener_{false};

    SolicitudTurno solicitudActual_;
    ResultadoRafaga resultadoActual_;
};
