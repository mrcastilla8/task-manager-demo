#pragma once

#include "concurrent/ProcessThread.h"
#include "contracts/v2/IExecutionEngine.h"
#include "core/v2/AdmisorProcesos.h"
#include "core/v2/ListaProcesos.h"
#include "core/v2/Memoria.h"
#include "scheduler/SchedulerV2.h"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

namespace ConcurrentSimulator::v2 {

// Solo pruebas: etapa del alta en la que se inyecta un fallo (std::runtime_error)
// para verificar el rollback. Ver ConcurrentEngineV2::configurarFalloAltaPruebas().
enum class EtapaAlta {
    NINGUNA,
    CREAR_WORKER,
    CLASIFICAR,
    PREPARAR_EVENTOS,
    REGISTRAR_WORKER,
    ASIGNAR_TICKET
};

// Motor concurrente v2 (Integrante 4). Un CPU logico unico atendido por workers
// reales: cada proceso admitido tiene un hilo que calcula su rafaga y un hilo
// coordinador serializa los pasos. Usa directamente los modulos compartidos:
//   - core/v2: ListaProcesos, Memoria y admitirProceso (Castilla), unica
//     fuente del almacenamiento de procesos y prioridades;
//   - scheduler/SchedulerV2: clasificacion P1-P3, seleccion y AsignadorTickets
//     (Brickzon).
// No depende de SimulationEngineV2: reloj, transiciones, eventos y timeline son
// responsabilidad de este motor.
//
// Ejecucion continua: hasta que G1 ratifique su transporte, iniciar() solo fija
// el estado EJECUTANDO, igual que el motor logico. El avance es siguientePaso()
// con el motor PAUSADO.
class ConcurrentEngineV2 final : public IExecutionEngine {
public:
    explicit ConcurrentEngineV2(Configuracion configuracion);
    ~ConcurrentEngineV2() override;

    ConcurrentEngineV2(const ConcurrentEngineV2&) = delete;
    ConcurrentEngineV2& operator=(const ConcurrentEngineV2&) = delete;

    Resultado agregarProceso(const AltaProceso& proceso) override;
    Resultado cancelarProceso(int pid) override;
    Resultado iniciar() override;
    Resultado pausar() override;
    Resultado reiniciar() override;
    ResultadoPaso siguientePaso() override;
    EstadoSistema obtenerEstado() const override;

    std::size_t cantidadWorkers() const;

    // ---- Solo pruebas; desactivados por defecto ----------------------------
    // Retiene cada rafaga en su worker hasta liberarRafagaPruebas() (con limite
    // maximo). No es un retardo de CPU: no interviene en el reloj ni en el CPU.
    void activarPuertaRafagaPruebas(bool activa);
    // Libera la rafaga del proceso en ejecucion; sin efecto si no hay ninguno.
    void liberarRafagaPruebas();
    void configurarFalloAltaPruebas(EtapaAlta etapa);
    void fijarSiguienteTicketPruebas(std::uint64_t valor);

private:
    struct SolicitudPaso {
        ResultadoPaso resultado{CodigoResultado::ESTADO_INVALIDO, false, false, 0};
        bool listo{false};
    };

    struct Espera {
        int pid{-1};
        std::int64_t despertarMs{0};
    };

    void loopCoordinador();

    // Todos los metodos *Interno exigen mutex_ tomado por el llamador.
    ResultadoPaso ejecutarPasoInterno(std::unique_lock<std::mutex>& lock);
    ResultadoPaso ejecutarIdleInterno();
    bool despertarEsperasVencidasInterno();
    void cancelarInterno(int pid);
    void reclasificarInterno();
    void registrarInterno(TipoEvento tipo, int pid = -1,
                          std::int64_t duracionMs = 0);
    void reservarEventosInterno(std::size_t adicionales);
    void retirarWorkerInterno(int pid);
    void abortarTurnoInterno() noexcept;
    void liberarMemoriaInterno(int pid) noexcept;
    bool hayActivoInterno() const;
    void finalizarSiCorrespondeInterno();
    std::uint64_t ticketsDisponiblesInterno() const noexcept;
    bool puedeAvanzarRelojInterno(std::int64_t duracionMs) const noexcept;
    void inyectarFalloAlta(EtapaAlta etapa) const;

    Configuracion configuracion_;

    mutable std::mutex mutex_;
    std::condition_variable cvCoordinador_;
    std::condition_variable cvPaso_;
    std::deque<std::shared_ptr<SolicitudPaso>> solicitudes_;
    bool pasoEnCurso_{false};
    bool reiniciando_{false};
    bool destruir_{false};

    ListaProcesos procesos_;
    Memoria memoria_;
    Scheduler scheduler_;
    AsignadorTickets asignadorTickets_;
    std::map<int, std::unique_ptr<::ProcessThread>> workers_;

    std::int64_t relojMs_{0};
    EstadoEjecucion estado_{EstadoEjecucion::DETENIDO};
    int pidEjecutando_{-1};
    std::int64_t cuotaTurnoFijadaMs_{0};
    std::int64_t cpuConsumidaTurnoMs_{0};
    std::vector<Espera> esperas_;
    std::unordered_set<int> ioConsumida_;
    std::unordered_set<int> cancelacionesPendientes_;
    std::vector<Evento> eventos_;
    std::vector<BloqueTimeline> timeline_;

    bool puertaActiva_{false};
    EtapaAlta falloAltaPruebas_{EtapaAlta::NINGUNA};

    // Debe ser el ultimo miembro: el hilo arranca al construirlo y lee los
    // demas miembros, que deben estar inicializados antes.
    std::thread coordinador_;
};

}  // namespace ConcurrentSimulator::v2
