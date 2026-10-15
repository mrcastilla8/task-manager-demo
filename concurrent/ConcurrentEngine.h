#pragma once

#include "concurrent/PlanificadorMotor.h"
#include "concurrent/ProcessThread.h"
#include "concurrent/Synchronization.h"
#include "contracts/EstadoSistema.h"
#include "contracts/IExecutionEngine.h"
#include "contracts/Proceso.h"
#include "core/ColaProcesos.h"
#include "core/Memoria.h"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Solo pruebas: etapa del alta en la que se inyecta un fallo para verificar el
// rollback. Ver ConcurrentEngine::configurarFalloAltaPruebas().
enum class EtapaAlta {
    NINGUNA,
    CREAR_WORKER,
    INSERTAR_COLA,
    REGISTRAR_WORKER,
    CALCULAR_COLA,
    PREPARAR_EVENTOS
};

class ConcurrentEngine : public IExecutionEngine {
public:
    explicit ConcurrentEngine(int capacidadMemoriaBytes = 1024 * 1024);
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

    // Solo pruebas: retardo fisico por rafaga dentro del worker (0 = desactivado).
    // No modifica el tiempo simulado ni la cuota.
    void configurarRetardoRafagaPruebasMs(int ms) noexcept;

    // Solo pruebas: hace fallar el siguiente alta en la etapa indicada (lanza
    // std::runtime_error). NINGUNA lo desactiva. Permite comprobar que un fallo
    // a mitad del alta no deja memoria reservada, procesos ni workers a medias.
    void configurarFalloAltaPruebas(EtapaAlta etapa);

    // Solo pruebas: fija el proximo ticket para probar el agotamiento del
    // contador sin recorrer 2^63 ingresos a READY.
    void fijarSiguienteTicketPruebas(std::int64_t valor);

private:
    struct InfoIO {
        bool operacionRealizada{false};
        std::int64_t tiempoEsperaRestanteMs{0};
    };

    void loopCoordinador();
    void ejecutarPasoInterno(std::unique_lock<std::mutex>& lock);
    void registrarEventoInterno(TipoEvento tipo, int pid, std::string descripcion);
    void detenerTodosLosWorkersInterno();
    void cancelarProcesoInterno(
        Proceso& proceso,
        const char* descripcion = "Proceso cancelado por accion del usuario");
    Evento construirEvento(TipoEvento tipo, int pid, std::string descripcion) const;
    std::int64_t cuotaParaDespachoInterno(const Proceso& proceso) const noexcept;
    void inyectarFalloAlta(EtapaAlta etapa) const;
    std::int64_t asignarTicketInterno();
    void exigirTicketsDisponiblesInterno(std::int64_t cantidad) const;

    // Los siguientes metodos exigen mutexEstado_ tomado por el llamador.
    bool todosTerminadosInterno() const;
    void actualizarColaPrioridadInterno();
    void avanzarEsperasIOInterno(std::int64_t delta);
    void acumularEsperaListosInterno(std::int64_t delta);

    mutable std::mutex mutexEstado_;
    std::condition_variable cvCoordinador_;
    std::condition_variable cvPasoCompletado_;
    bool pasoEnCurso_{false};
    bool reiniciando_{false};

    ColaProcesos colaProcesos_;
    Memoria memoria_;
    PlanificadorMotor planificador_;

    std::unordered_map<int, std::unique_ptr<ProcessThread>> workers_;
    std::unordered_map<int, InfoIO> trackingIO_;
    // Cancelaciones solicitadas sobre un proceso en RUN: se resuelven al cerrar
    // la rafaga, tras contabilizar el CPU completado.
    std::unordered_set<int> cancelacionesPendientes_;
    // Ticket estrictamente creciente asignado en cada ingreso a READY (alta,
    // retorno de WAIT y fin de cuota). Se almacena en Proceso::ordenLlegada.
    std::int64_t siguienteTicket_{1};
    EtapaAlta falloAltaPruebas_{EtapaAlta::NINGUNA};

    std::int64_t tiempoSimulado_{0};
    EstadoEjecucion estadoEjecucion_{EstadoEjecucion::DETENIDO};
    std::int64_t quantumMs_{ConfiguracionPredeterminada::QUANTUM_MS};
    int pidEjecutando_{-1};

    std::vector<int> colaPrioridad_;
    std::vector<Evento> eventos_;
    std::vector<BloqueTimeline> timeline_;

    bool destruirCoordinador_{false};
    std::atomic<int> intervaloCoordinadorMs_{10};
    std::atomic<int> retardoRafagaPruebasMs_{0};

    // Debe ser el ultimo miembro: el hilo arranca al construirlo y lee los
    // demas miembros, que deben estar inicializados antes.
    std::thread hiloCoordinador_;
};
