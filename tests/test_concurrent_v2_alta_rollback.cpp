// Alta transaccional del motor concurrente v2: un fallo en cualquier etapa no
// deja memoria reservada, procesos incompletos, workers huerfanos ni tickets
// consumidos, y no altera las prioridades de los demas procesos.
#include "tests/ConcurrentV2Support.h"
#include "tests/TestSupport.h"

#include <stdexcept>
#include <vector>

using namespace ConcurrentSimulator::v2;
using namespace ConcurrentSimulator::v2::testing;

int main() {
    const std::vector<EtapaAlta> etapas = {
        EtapaAlta::CREAR_WORKER, EtapaAlta::CLASIFICAR,
        EtapaAlta::PREPARAR_EVENTOS, EtapaAlta::REGISTRAR_WORKER,
        EtapaAlta::ASIGNAR_TICKET};

    // Caso 1: fallo inyectado en cada etapa del alta.
    for (const EtapaAlta etapa : etapas) {
        ConcurrentEngineV2 engine(Configuracion{1000});
        // Dos procesos previos: el nuevo (enorme) cambiaria sus prioridades.
        EXPECT_TRUE(engine.agregarProceso(alta(1, 200, 80)));
        EXPECT_TRUE(engine.agregarProceso(alta(2, 100, 80)));
        const EstadoSistema antes = engine.obtenerEstado();
        EXPECT_EQ(engine.cantidadWorkers(), 2U);

        engine.configurarFalloAltaPruebas(etapa);
        EXPECT_THROW(engine.agregarProceso(alta(3, 600, 60)), std::runtime_error);

        const EstadoSistema tras = engine.obtenerEstado();
        EXPECT_TRUE(diferencia(antes, tras).empty());  // estado completo intacto
        EXPECT_EQ(tras.memoriaUsadaBytes, 300);
        EXPECT_EQ(engine.cantidadWorkers(), 2U);
        EXPECT_TRUE(buscar(tras, 3) == nullptr);
        EXPECT_TRUE(prioridadesCoherentes(tras));
        EXPECT_EQ(invariantesVioladas(tras), 0);

        // El mismo PID puede admitirse despues y recibe el ticket siguiente: el
        // intento fallido no consumio ninguno.
        engine.configurarFalloAltaPruebas(EtapaAlta::NINGUNA);
        EXPECT_TRUE(engine.agregarProceso(alta(3, 600, 60)));
        const EstadoSistema ok = engine.obtenerEstado();
        EXPECT_EQ(buscar(ok, 1)->ticketReady, 1U);
        EXPECT_EQ(buscar(ok, 2)->ticketReady, 2U);
        EXPECT_EQ(buscar(ok, 3)->ticketReady, 3U);
        EXPECT_EQ(ok.memoriaUsadaBytes, 900);
        EXPECT_EQ(engine.cantidadWorkers(), 3U);
        EXPECT_TRUE(prioridadesCoherentes(ok));

        // Los tres procesos terminan y la memoria vuelve a cero.
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());
        for (int i = 0; i < 100 &&
                        engine.obtenerEstado().estadoEjecucion !=
                            EstadoEjecucion::FINALIZADO;
             ++i) {
            EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
        }
        const EstadoSistema fin = engine.obtenerEstado();
        EXPECT_EQ(fin.estadoEjecucion, EstadoEjecucion::FINALIZADO);
        EXPECT_EQ(fin.memoriaUsadaBytes, 0);
        EXPECT_EQ(engine.cantidadWorkers(), 0U);
    }

    // Caso 2: un alta fallida no impide el reinicio ni deja workers colgando.
    {
        ConcurrentEngineV2 engine(Configuracion{1000});
        engine.configurarFalloAltaPruebas(EtapaAlta::ASIGNAR_TICKET);
        EXPECT_THROW(engine.agregarProceso(alta(1, 100, 50)), std::runtime_error);
        EXPECT_EQ(engine.cantidadWorkers(), 0U);
        EXPECT_EQ(engine.obtenerEstado().memoriaUsadaBytes, 0);
        EXPECT_TRUE(engine.reiniciar());
        EXPECT_EQ(engine.cantidadWorkers(), 0U);
        EXPECT_EQ(engine.obtenerEstado().procesos.size(), 0U);
    }

    // Caso 3: con el motor en marcha, altas fallidas repetidas no alteran el
    // avance de los demas procesos.
    {
        ConcurrentEngineV2 engine(Configuracion{10000});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 100, 300)));
        EXPECT_TRUE(engine.iniciar());
        EXPECT_TRUE(engine.pausar());
        EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);

        engine.configurarFalloAltaPruebas(EtapaAlta::REGISTRAR_WORKER);
        for (int pid = 2; pid <= 6; ++pid) {
            EXPECT_THROW(engine.agregarProceso(alta(pid, 100, 50)),
                         std::runtime_error);
            EXPECT_EQ(invariantesVioladas(engine.obtenerEstado()), 0);
        }
        engine.configurarFalloAltaPruebas(EtapaAlta::NINGUNA);

        while (engine.obtenerEstado().estadoEjecucion !=
               EstadoEjecucion::FINALIZADO) {
            EXPECT_EQ(engine.siguientePaso().codigo, CodigoResultado::OK);
        }
        const EstadoSistema s = engine.obtenerEstado();
        EXPECT_EQ(s.procesos.size(), 1U);
        EXPECT_EQ(s.tiempoSimuladoMs, 300);
        EXPECT_EQ(s.memoriaUsadaBytes, 0);
    }

    // Caso 4: las altas rechazadas por validacion no consumen recursos ni tickets.
    {
        ConcurrentEngineV2 engine(Configuracion{500});
        EXPECT_TRUE(engine.agregarProceso(alta(1, 400, 50)));
        EXPECT_EQ(engine.agregarProceso(alta(0, 10, 50)).codigo,
                  CodigoResultado::ARGUMENTO_INVALIDO);
        EXPECT_EQ(engine.agregarProceso(alta(1, 50, 50)).codigo,
                  CodigoResultado::PID_DUPLICADO);
        EXPECT_EQ(engine.agregarProceso(alta(2, 200, 50)).codigo,
                  CodigoResultado::MEMORIA_INSUFICIENTE);
        EXPECT_EQ(engine.obtenerEstado().memoriaUsadaBytes, 400);
        EXPECT_EQ(engine.cantidadWorkers(), 1U);
        EXPECT_TRUE(engine.agregarProceso(alta(3, 50, 50)));
        EXPECT_EQ(buscar(engine.obtenerEstado(), 3)->ticketReady, 2U);
    }

    return test::finish();
}
