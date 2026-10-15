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

    // Solo pruebas: libera una rafaga retenida por SolicitudTurno::esperarLiberacion.
    // Si se llama antes de que el worker llegue a la puerta, la libera igualmente.
    void liberarRafaga();
    // Solo pruebas: sustituye el motivo del siguiente resultado de worker.
    void forzarMotivoProximoTurnoPruebas(MotivoFinTurno motivo);

    bool estaDetenido() const noexcept;

private:
    void ejecutarLoopWorker();

    int pid_;
    mutable std::mutex mutex_;
    std::condition_variable cvTurno_;
    std::condition_variable cvFinTurno_;

    bool permisoEjecucion_{false};
    bool turnoCompletado_{false};
    bool detener_{false};
    bool liberada_{false};
    MotivoFinTurno motivoForzadoPruebas_{MotivoFinTurno::NINGUNO};

    SolicitudTurno solicitudActual_;
    ResultadoRafaga resultadoActual_;

    // Debe declararse al final: el hilo arranca al construir este miembro y
    // necesita que el mutex, las variables de condicion y las banderas ya
    // existan. Tambien se destruye primero.
    std::thread worker_;
};
