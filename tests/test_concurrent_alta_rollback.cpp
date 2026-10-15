#include "concurrent/ConcurrentEngine.h"
#include "tests/Fixtures.h"
#include "tests/SnapshotInvariants.h"
#include "tests/TestSupport.h"

#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

namespace {

Proceso crear(int pid, int tamanio, std::int64_t total) {
    Proceso p = crearProceso(pid, "P" + std::to_string(pid), tamanio,
                             Estado::READY, 5, 0);
    p.tiempoTotal = total;
    p.tiempoRestante = total;
    return p;
}

const Proceso* buscar(const EstadoSistema& estado, int pid) {
    const auto it = std::find_if(
        estado.procesos.begin(), estado.procesos.end(),
        [pid](const Proceso& p) { return p.pid == pid; });
    return it == estado.procesos.end() ? nullptr : &*it;
}

}  // namespace

int main() {
    const std::vector<EtapaAlta> etapas = {
        EtapaAlta::CREAR_WORKER,     EtapaAlta::INSERTAR_COLA,
        EtapaAlta::REGISTRAR_WORKER, EtapaAlta::CALCULAR_COLA,
        EtapaAlta::PREPARAR_EVENTOS};

    // Caso 1: un fallo en cualquier etapa del alta no deja rastro alguno.
    for (const EtapaAlta etapa : etapas) {
        ConcurrentEngine engine(1000);
        engine.configurarQuantum(500);
        engine.agregarProceso(crear(1, 200, 800));

        const EstadoSistema antes = engine.obtenerEstado();
        EXPECT_EQ(engine.cantidadWorkersActivos(), 1);

        engine.configurarFalloAltaPruebas(etapa);
        EXPECT_THROW(engine.agregarProceso(crear(2, 300, 600)),
                     std::runtime_error);

        const EstadoSistema tras = engine.obtenerEstado();
        EXPECT_EQ(tras.memoriaUsada, antes.memoriaUsada);
        EXPECT_EQ(engine.memoriaUsada(), 200);
        EXPECT_EQ(engine.cantidadWorkersActivos(), 1);
        EXPECT_EQ(tras.procesos.size(), 1U);
        EXPECT_TRUE(buscar(tras, 2) == nullptr);
        EXPECT_EQ(tras.eventos.size(), antes.eventos.size());
        EXPECT_TRUE(tras.colaPrioridad == antes.colaPrioridad);
        EXPECT_EQ(invariantesVioladas(tras), 0);

        // El mismo PID puede admitirse despues y recibe el ticket siguiente: el
        // intento fallido no consumio ninguno.
        engine.configurarFalloAltaPruebas(EtapaAlta::NINGUNA);
        engine.agregarProceso(crear(2, 300, 600));
        const EstadoSistema ok = engine.obtenerEstado();
        EXPECT_EQ(ok.procesos.size(), 2U);
        EXPECT_EQ(buscar(ok, 1)->ordenLlegada, 1);
        EXPECT_EQ(buscar(ok, 2)->ordenLlegada, 2);
        EXPECT_EQ(ok.memoriaUsada, 500);
        EXPECT_EQ(engine.cantidadWorkersActivos(), 2);
        EXPECT_EQ(ok.eventos.size(), antes.eventos.size() + 3);
        EXPECT_EQ(invariantesVioladas(ok), 0);

        // Ambos procesos terminan y la memoria vuelve a cero.
        engine.pausar();
        for (int i = 0; i < 10 &&
                        engine.obtenerEstado().estadoEjecucion !=
                            EstadoEjecucion::FINALIZADO;
             ++i) {
            engine.siguientePaso();
        }
        const EstadoSistema fin = engine.obtenerEstado();
        EXPECT_EQ(fin.estadoEjecucion, EstadoEjecucion::FINALIZADO);
        EXPECT_EQ(fin.memoriaUsada, 0);
        EXPECT_EQ(buscar(fin, 1)->estado, Estado::FINISH);
        EXPECT_EQ(buscar(fin, 2)->estado, Estado::FINISH);
    }

    // Caso 2: un alta fallida no impide el reinicio ni deja workers colgando.
    {
        ConcurrentEngine engine(1000);
        engine.configurarFalloAltaPruebas(EtapaAlta::REGISTRAR_WORKER);
        EXPECT_THROW(engine.agregarProceso(crear(1, 100, 500)),
                     std::runtime_error);
        EXPECT_EQ(engine.cantidadWorkersActivos(), 0);
        EXPECT_EQ(engine.memoriaUsada(), 0);
        engine.reiniciar();
        EXPECT_EQ(engine.cantidadWorkersActivos(), 0);
        EXPECT_EQ(engine.obtenerEstado().procesos.size(), 0U);
    }

    // Caso 3: con el motor en ejecucion continua, un alta fallida no altera el
    // avance de los demas procesos ni deja el CPU en un estado inconsistente.
    {
        ConcurrentEngine engine(10000);
        engine.configurarIntervaloCoordinadorMs(0);
        engine.configurarQuantum(100);
        engine.configurarRetardoRafagaPruebasMs(5);
        engine.agregarProceso(crear(1, 100, 1000));
        engine.iniciar();

        engine.configurarFalloAltaPruebas(EtapaAlta::CALCULAR_COLA);
        for (int i = 2; i <= 6; ++i) {
            EXPECT_THROW(engine.agregarProceso(crear(i, 100, 500)),
                         std::runtime_error);
            EXPECT_EQ(invariantesVioladas(engine.obtenerEstado()), 0);
        }
        engine.configurarFalloAltaPruebas(EtapaAlta::NINGUNA);

        const auto limite =
            std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (engine.obtenerEstado().estadoEjecucion !=
                   EstadoEjecucion::FINALIZADO &&
               std::chrono::steady_clock::now() < limite) {
            std::this_thread::yield();
        }

        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.estadoEjecucion, EstadoEjecucion::FINALIZADO);
        EXPECT_EQ(s.procesos.size(), 1U);
        EXPECT_EQ(s.memoriaUsada, 0);
        EXPECT_EQ(s.tiempoSimulado, 1000);
        EXPECT_EQ(engine.cantidadWorkersActivos(), 1);  // el worker de P1
    }

    // Caso 4: las validaciones previas no se ven afectadas y tampoco consumen
    // recursos (argumento invalido, PID duplicado y memoria insuficiente).
    {
        ConcurrentEngine engine(500);
        engine.agregarProceso(crear(1, 400, 500));

        Proceso invalido;
        EXPECT_THROW(engine.agregarProceso(invalido), std::invalid_argument);
        EXPECT_THROW(engine.agregarProceso(crear(1, 50, 500)),
                     std::invalid_argument);
        EXPECT_THROW(engine.agregarProceso(crear(2, 200, 500)),
                     std::runtime_error);

        EXPECT_EQ(engine.memoriaUsada(), 400);
        EXPECT_EQ(engine.cantidadWorkersActivos(), 1);
        EXPECT_EQ(engine.obtenerEstado().procesos.size(), 1U);

        // El ticket no se consumio con los intentos rechazados.
        engine.agregarProceso(crear(3, 50, 500));
        EXPECT_EQ(buscar(engine.obtenerEstado(), 3)->ordenLlegada, 2);
    }

    return test::finish();
}
