#include "concurrent/ProcessThread.h"

#include <algorithm>

ProcessThread::ProcessThread(int pid)
    : pid_(pid),
      worker_(&ProcessThread::ejecutarLoopWorker, this) {}

ProcessThread::~ProcessThread() {
    solicitarDetencion();
}

int ProcessThread::pid() const noexcept {
    return pid_;
}

void ProcessThread::concederTurno(const SolicitudTurno& solicitud) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        solicitudActual_ = solicitud;
        permisoEjecucion_ = true;
        turnoCompletado_ = false;
    }
    cvTurno_.notify_one();
}

ResultadoRafaga ProcessThread::esperarFinTurno() {
    std::unique_lock<std::mutex> lock(mutex_);
    cvFinTurno_.wait(lock, [this]() {
        return turnoCompletado_ || detener_;
    });

    if (detener_) {
        ResultadoRafaga res;
        res.pid = pid_;
        res.motivo = MotivoFinTurno::CANCELADO;
        return res;
    }

    turnoCompletado_ = false;
    return resultadoActual_;
}

void ProcessThread::solicitarDetencion() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (detener_) {
            return;
        }
        detener_ = true;
        permisoEjecucion_ = false;
    }
    cvTurno_.notify_all();
    cvFinTurno_.notify_all();

    if (worker_.joinable()) {
        worker_.join();
    }
}

bool ProcessThread::estaDetenido() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return detener_;
}

void ProcessThread::ejecutarLoopWorker() {
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        cvTurno_.wait(lock, [this]() {
            return permisoEjecucion_ || detener_;
        });

        if (detener_) {
            break;
        }

        // Calculo de rafaga logica dentro del worker thread real
        ResultadoRafaga resultado;
        resultado.pid = pid_;

        const std::int64_t tiempoPorEjecutar =
            std::min(solicitudActual_.cuotaMs, solicitudActual_.tiempoRestante);

        if (solicitudActual_.tieneIO && solicitudActual_.cpuRestanteParaIO > 0 &&
            solicitudActual_.cpuRestanteParaIO <= tiempoPorEjecutar) {
            resultado.tiempoEjecutadoMs = solicitudActual_.cpuRestanteParaIO;
            resultado.motivo = MotivoFinTurno::BLOQUEO_IO;
        } else if (tiempoPorEjecutar >= solicitudActual_.tiempoRestante) {
            resultado.tiempoEjecutadoMs = solicitudActual_.tiempoRestante;
            resultado.motivo = MotivoFinTurno::PROCESO_TERMINADO;
        } else {
            resultado.tiempoEjecutadoMs = tiempoPorEjecutar;
            resultado.motivo = MotivoFinTurno::QUANTUM_AGOTADO;
        }

        resultadoActual_ = resultado;
        permisoEjecucion_ = false;
        turnoCompletado_ = true;

        lock.unlock();
        cvFinTurno_.notify_one();
    }
}
