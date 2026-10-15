#include "simulation/SimulationEngineV2.h"
#include "tests/TestSupport.h"

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace ConcurrentSimulator::v2 {

struct SimulationEngineV2TestAccess {
    static void fijarProximoTicket(SimulationEngineV2& motor,
                                   std::uint64_t siguiente) {
        motor.asignadorTickets_.reiniciar(siguiente);
    }

    static std::uint64_t proximoTicket(const SimulationEngineV2& motor) {
        return motor.asignadorTickets_.proximo();
    }

    static void fallarTrasEventos(SimulationEngineV2& motor) {
        motor.puntoFalloPrueba_ =
            SimulationEngineV2::PuntoFalloPrueba::TRAS_EVENTOS_ADMISION;
    }

    static void fallarDuranteReclasificacion(SimulationEngineV2& motor) {
        motor.puntoFalloPrueba_ =
            SimulationEngineV2::PuntoFalloPrueba::DURANTE_RECLASIFICACION;
    }
};

}  // namespace ConcurrentSimulator::v2

namespace {
using namespace ConcurrentSimulator::v2;

void expectSnapshotIgual(const EstadoSistema& antes,
                         const EstadoSistema& despues) {
    EXPECT_EQ(despues.tiempoSimuladoMs, antes.tiempoSimuladoMs);
    EXPECT_EQ(despues.estadoEjecucion, antes.estadoEjecucion);
    EXPECT_EQ(despues.colaReady, antes.colaReady);
    EXPECT_EQ(despues.pidEjecutando, antes.pidEjecutando);
    EXPECT_EQ(despues.cuotaTurnoFijadaMs, antes.cuotaTurnoFijadaMs);
    EXPECT_EQ(despues.cpuConsumidaTurnoMs, antes.cpuConsumidaTurnoMs);
    EXPECT_EQ(despues.memoriaUsadaBytes, antes.memoriaUsadaBytes);
    EXPECT_EQ(despues.memoriaTotalBytes, antes.memoriaTotalBytes);
    EXPECT_EQ(despues.procesos.size(), antes.procesos.size());
    for (std::size_t i = 0; i < antes.procesos.size(); ++i) {
        EXPECT_EQ(despues.procesos[i].pid, antes.procesos[i].pid);
        EXPECT_EQ(despues.procesos[i].nombre, antes.procesos[i].nombre);
        EXPECT_EQ(despues.procesos[i].tamanioBytes,
                  antes.procesos[i].tamanioBytes);
        EXPECT_EQ(despues.procesos[i].tiempoCpuTotalMs,
                  antes.procesos[i].tiempoCpuTotalMs);
        EXPECT_EQ(despues.procesos[i].estado, antes.procesos[i].estado);
        EXPECT_EQ(despues.procesos[i].prioridad, antes.procesos[i].prioridad);
        EXPECT_EQ(despues.procesos[i].ticketReady,
                  antes.procesos[i].ticketReady);
        EXPECT_EQ(despues.procesos[i].tiempoCpuConsumidoMs,
                  antes.procesos[i].tiempoCpuConsumidoMs);
        EXPECT_EQ(despues.procesos[i].tiempoCpuRestanteMs,
                  antes.procesos[i].tiempoCpuRestanteMs);
        EXPECT_EQ(despues.procesos[i].tiempoEsperaMs,
                  antes.procesos[i].tiempoEsperaMs);
        EXPECT_EQ(despues.procesos[i].operacionIO.habilitada,
                  antes.procesos[i].operacionIO.habilitada);
        EXPECT_EQ(despues.procesos[i].operacionIO.despuesDeCpuMs,
                  antes.procesos[i].operacionIO.despuesDeCpuMs);
        EXPECT_EQ(despues.procesos[i].operacionIO.duracionEsperaMs,
                  antes.procesos[i].operacionIO.duracionEsperaMs);
    }
    EXPECT_EQ(despues.eventos.size(), antes.eventos.size());
    for (std::size_t i = 0; i < antes.eventos.size(); ++i) {
        EXPECT_EQ(despues.eventos[i].tiempoSimuladoMs,
                  antes.eventos[i].tiempoSimuladoMs);
        EXPECT_EQ(despues.eventos[i].tipo, antes.eventos[i].tipo);
        EXPECT_EQ(despues.eventos[i].pid, antes.eventos[i].pid);
        EXPECT_EQ(despues.eventos[i].duracionMs,
                  antes.eventos[i].duracionMs);
        EXPECT_EQ(despues.eventos[i].detalle, antes.eventos[i].detalle);
    }
    EXPECT_EQ(despues.timeline.size(), antes.timeline.size());
    for (std::size_t i = 0; i < antes.timeline.size(); ++i) {
        EXPECT_EQ(despues.timeline[i].pid(), antes.timeline[i].pid());
        EXPECT_EQ(despues.timeline[i].inicioMs(), antes.timeline[i].inicioMs());
        EXPECT_EQ(despues.timeline[i].finMs(), antes.timeline[i].finMs());
    }
}
}  // namespace

int main() {
    using namespace ConcurrentSimulator::v2;
    const auto maxTicket = (std::numeric_limits<std::uint64_t>::max)();

    // Un alta rechazada por agotamiento no publica proceso, reserva, ticket,
    // clasificación ni eventos.
    SimulationEngineV2 alta(Configuracion{256});
    EXPECT_EQ(alta.agregarProceso({1, "existente", 20, 30, {}}).codigo,
              CodigoResultado::OK);
    const EstadoSistema antesAlta = alta.obtenerEstado();
    SimulationEngineV2TestAccess::fijarProximoTicket(alta, maxTicket);
    const Resultado rechazo =
        alta.agregarProceso({2, "rechazado", 80, 40, {}});
    EXPECT_EQ(rechazo.codigo, CodigoResultado::ESTADO_INVALIDO);
    expectSnapshotIgual(antesAlta, alta.obtenerEstado());
    EXPECT_EQ(SimulationEngineV2TestAccess::proximoTicket(alta), maxTicket);

    // Fallo controlado después de preparar eventos: la excepción no confirma
    // proceso, reserva, ticket ni publicaciones parciales.
    SimulationEngineV2 falloClasificacion(Configuracion{256});
    EXPECT_EQ(falloClasificacion
                  .agregarProceso({10, "base", 20, 30, {}})
                  .codigo,
              CodigoResultado::OK);
    const EstadoSistema antesClasificacion = falloClasificacion.obtenerEstado();
    SimulationEngineV2TestAccess::fallarTrasEventos(falloClasificacion);
    EXPECT_THROW(falloClasificacion.agregarProceso(
                     {11, "no confirmado", 80, 40, {}}),
                 std::runtime_error);
    expectSnapshotIgual(antesClasificacion,
                        falloClasificacion.obtenerEstado());

    SimulationEngineV2 falloPrioridades(Configuracion{256});
    EXPECT_EQ(falloPrioridades.agregarProceso({15, "base", 20, 30, {}}).codigo,
              CodigoResultado::OK);
    const EstadoSistema antesPrioridades = falloPrioridades.obtenerEstado();
    SimulationEngineV2TestAccess::fallarDuranteReclasificacion(falloPrioridades);
    EXPECT_THROW(falloPrioridades.agregarProceso(
                     {16, "no clasificado", 80, 40, {}}),
                 std::runtime_error);
    expectSnapshotIgual(antesPrioridades, falloPrioridades.obtenerEstado());

    // Al avanzar idle hasta el instante exacto de despertar, el ticket no
    // disponible se detecta antes de publicar reloj o eventos.
    SimulationEngineV2 idle(Configuracion{128});
    EXPECT_EQ(idle.agregarProceso(
                  {20, "espera", 10, 10, {true, 1, 5}})
                  .codigo,
              CodigoResultado::OK);
    EXPECT_TRUE(idle.iniciar());
    EXPECT_TRUE(idle.pausar());
    const ResultadoPaso entraEnWait = idle.siguientePaso();
    EXPECT_TRUE(entraEnWait.ejecutoCpu);
    EXPECT_EQ(entraEnWait.tiempoAvanzadoMs, 1);
    const EstadoSistema antesDespertar = idle.obtenerEstado();
    EXPECT_EQ(antesDespertar.tiempoSimuladoMs, 1);
    EXPECT_EQ(antesDespertar.procesos[0].estado, Estado::WAIT);
    SimulationEngineV2TestAccess::fijarProximoTicket(idle, maxTicket);
    const ResultadoPaso despertarFallido = idle.siguientePaso();
    EXPECT_EQ(despertarFallido.codigo, CodigoResultado::ESTADO_INVALIDO);
    EXPECT_FALSE(despertarFallido.avanzo);
    EXPECT_EQ(SimulationEngineV2TestAccess::proximoTicket(idle), maxTicket);
    expectSnapshotIgual(antesDespertar, idle.obtenerEstado());

    return test::finish();
}
