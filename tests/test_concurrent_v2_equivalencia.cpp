#include "concurrent/ConcurrentEngineV2.h"
#include "simulation/SimulationEngineV2.h"
#include "tests/TestSupport.h"

#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

using namespace ConcurrentSimulator::v2;

namespace {

bool mismoEstado(const EstadoSistema& a, const EstadoSistema& b) {
    if (a.tiempoSimuladoMs != b.tiempoSimuladoMs ||
        a.estadoEjecucion != b.estadoEjecucion ||
        a.colaReady != b.colaReady || a.pidEjecutando != b.pidEjecutando ||
        a.cuotaTurnoFijadaMs != b.cuotaTurnoFijadaMs ||
        a.cpuConsumidaTurnoMs != b.cpuConsumidaTurnoMs ||
        a.memoriaUsadaBytes != b.memoriaUsadaBytes ||
        a.memoriaTotalBytes != b.memoriaTotalBytes ||
        a.procesos.size() != b.procesos.size() ||
        a.eventos.size() != b.eventos.size() ||
        a.timeline.size() != b.timeline.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.procesos.size(); ++i) {
        const Proceso& x = a.procesos[i];
        const Proceso& y = b.procesos[i];
        if (x.pid != y.pid || x.estado != y.estado ||
            x.prioridad != y.prioridad || x.ticketReady != y.ticketReady ||
            x.tiempoCpuConsumidoMs != y.tiempoCpuConsumidoMs ||
            x.tiempoCpuRestanteMs != y.tiempoCpuRestanteMs ||
            x.tiempoEsperaMs != y.tiempoEsperaMs) {
            return false;
        }
    }
    for (std::size_t i = 0; i < a.eventos.size(); ++i) {
        if (a.eventos[i].tiempoSimuladoMs != b.eventos[i].tiempoSimuladoMs ||
            a.eventos[i].tipo != b.eventos[i].tipo ||
            a.eventos[i].pid != b.eventos[i].pid ||
            a.eventos[i].duracionMs != b.eventos[i].duracionMs ||
            a.eventos[i].detalle != b.eventos[i].detalle) {
            return false;
        }
    }
    for (std::size_t i = 0; i < a.timeline.size(); ++i) {
        if (a.timeline[i].pid() != b.timeline[i].pid() ||
            a.timeline[i].inicioMs() != b.timeline[i].inicioMs() ||
            a.timeline[i].finMs() != b.timeline[i].finMs()) {
            return false;
        }
    }
    return true;
}

}  // namespace

int main() {
    SimulationEngineV2 logico(Configuracion{250});
    ConcurrentEngineV2 concurrente(Configuracion{250});

    const AltaProceso altas[] = {
        {1, "pequeno", 12, 30, {true, 10, 5}},
        {2, "mediano", 40, 50, {}},
        {3, "grande", 96, 5, {}}
    };
    for (const AltaProceso& alta : altas) {
        EXPECT_TRUE(logico.agregarProceso(alta));
        EXPECT_TRUE(concurrente.agregarProceso(alta));
        EXPECT_TRUE(mismoEstado(logico.obtenerEstado(),
                                concurrente.obtenerEstado()));
    }
    EXPECT_EQ(concurrente.cantidadWorkers(), 3U);

    EXPECT_TRUE(logico.iniciar());
    EXPECT_TRUE(concurrente.iniciar());
    EXPECT_TRUE(logico.pausar());
    EXPECT_TRUE(concurrente.pausar());
    EXPECT_TRUE(mismoEstado(logico.obtenerEstado(), concurrente.obtenerEstado()));

    int pasosCPU = 0;
    for (int intento = 0; intento < 12; ++intento) {
        const ResultadoPaso esperado = logico.siguientePaso();
        const ResultadoPaso real = concurrente.siguientePaso();
        EXPECT_EQ(real.codigo, esperado.codigo);
        EXPECT_EQ(real.avanzo, esperado.avanzo);
        EXPECT_EQ(real.ejecutoCpu, esperado.ejecutoCpu);
        EXPECT_EQ(real.tiempoAvanzadoMs, esperado.tiempoAvanzadoMs);
        if (real.ejecutoCpu) {
            ++pasosCPU;
        }
        EXPECT_TRUE(mismoEstado(logico.obtenerEstado(),
                                concurrente.obtenerEstado()));
        if (concurrente.obtenerEstado().estadoEjecucion ==
            EstadoEjecucion::FINALIZADO) {
            break;
        }
    }
    EXPECT_EQ(pasosCPU, 5);
    EXPECT_EQ(concurrente.cantidadWorkers(), 0U);

    EXPECT_TRUE(logico.reiniciar());
    EXPECT_TRUE(concurrente.reiniciar());
    EXPECT_TRUE(mismoEstado(logico.obtenerEstado(), concurrente.obtenerEstado()));
    EXPECT_EQ(concurrente.cantidadWorkers(), 0U);

    EXPECT_TRUE(concurrente.agregarProceso({4, "cancelable", 24, 20, {}}));
    EXPECT_EQ(concurrente.cantidadWorkers(), 1U);
    EXPECT_TRUE(concurrente.cancelarProceso(4));
    EXPECT_EQ(concurrente.cantidadWorkers(), 0U);
    EXPECT_TRUE(concurrente.reiniciar());

    ConcurrentEngineV2 snapshotsConcurrentes(Configuracion{250});
    for (int pid = 10; pid < 15; ++pid) {
        EXPECT_TRUE(snapshotsConcurrentes.agregarProceso(
            {pid, "snapshot", 20 + pid, 100, {}}));
    }
    EXPECT_TRUE(snapshotsConcurrentes.iniciar());
    EXPECT_TRUE(snapshotsConcurrentes.pausar());
    std::atomic<bool> snapshotsValidos{true};
    std::vector<std::thread> lectores;
    for (int lector = 0; lector < 4; ++lector) {
        lectores.emplace_back([&snapshotsConcurrentes, &snapshotsValidos]() {
            for (int indice = 0; indice < 100; ++indice) {
                const EstadoSistema estado = snapshotsConcurrentes.obtenerEstado();
                if (estado.memoriaUsadaBytes != 160 ||
                    estado.memoriaTotalBytes != 250 ||
                    estado.procesos.size() != 5 || estado.colaReady.size() != 5) {
                    snapshotsValidos = false;
                }
            }
        });
    }
    for (std::thread& lector : lectores) lector.join();
    EXPECT_TRUE(snapshotsValidos.load());
    EXPECT_TRUE(snapshotsConcurrentes.cancelarProceso(12));
    EXPECT_EQ(snapshotsConcurrentes.obtenerEstado().memoriaUsadaBytes, 128);
    EXPECT_TRUE(snapshotsConcurrentes.reiniciar());
    EXPECT_EQ(snapshotsConcurrentes.cantidadWorkers(), 0U);

    return test::finish();
}
