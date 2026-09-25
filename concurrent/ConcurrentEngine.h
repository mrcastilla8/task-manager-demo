#pragma once

#include "concurrent/ProcessThread.h"
#include "concurrent/Synchronization.h"
#include "contracts/EstadoSistema.h"
#include "contracts/IExecutionEngine.h"
#include "contracts/Proceso.h"
#include "core/ColaProcesos.h"
#include "core/Memoria.h"
#include "scheduler/Scheduler.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

class ConcurrentEngine : public IExecutionEngine {
public:
    explicit ConcurrentEngine(
        int capacidadMemoriaBytes = 1024 * 1024,
        SchedulerConfig schedulerConfig = {});
    ~ConcurrentEngine() override;

    ConcurrentEngine(const ConcurrentEngine&) = delete;
    ConcurrentEngine& operator=(const ConcurrentEngine&) = delete;
    ConcurrentEngine(ConcurrentEngine&&) = delete;
    ConcurrentEngine& operator=(ConcurrentEngine&&) = delete;

    void agregarProceso(const Proceso& proceso) override;
    void cancelarProceso(int pid) override;

    void iniciar() override;
    void pausar() override;
    void reiniciar() override;
    void siguientePaso() override;

    void configurarQuantum(std::int64_t quantumMs) override;
    std::int64_t obtenerQuantum() const override;

    EstadoSistema obtenerEstado() const override;

    // Metodos auxiliares de configuracion e inspeccion
    void configurarIntervaloCoordinadorMs(int ms) noexcept;
    int obtenerIntervaloCoordinadorMs() const noexcept;

    int memoriaTotal() const noexcept;
    int memoriaUsada() const noexcept;
    int cantidadWorkersActivos() const;

private:
    struct InfoIO {
        bool operacionRealizada{false};
        std::int64_t tiempoEsperaRestanteMs{0};
    };

    void loopCoordinador();
    void ejecutarPasoInterno(std::unique_lock<std::mutex>& lock);
    void registrarEventoInterno(TipoEvento tipo, int pid, std::string descripcion);
    void detenerTodosLosWorkersInterno();

    mutable std::mutex mutexEstado_;
    std::condition_variable cvCoordinador_;
    std::condition_variable cvPasoCompletado_;
    bool pasoEnCurso_{false};

    ColaProcesos colaProcesos_;
    Memoria memoria_;
    Scheduler scheduler_;

    std::unordered_map<int, std::unique_ptr<ProcessThread>> workers_;
    std::unordered_map<int, InfoIO> trackingIO_;

    std::int64_t tiempoSimulado_{0};
    EstadoEjecucion estadoEjecucion_{EstadoEjecucion::DETENIDO};
    std::int64_t quantumMs_{ConfiguracionPredeterminada::QUANTUM_MS};
    int pidEjecutando_{-1};

    std::vector<int> colaPrioridad_;
    std::vector<Evento> eventos_;
    std::vector<BloqueTimeline> timeline_;

    std::thread hiloCoordinador_;
    bool destruirCoordinador_{false};
    int intervaloCoordinadorMs_{10};
};
