#include "gui/EngineSession.h"
#include "tests/TestSupport.h"

#include <future>
#include <thread>

using namespace ConcurrentSimulator::v2;

int main() {
    gui::EngineSession sesion(gui::EngineSession::crearLogico(128));
    const std::thread::id hiloUI = std::this_thread::get_id();

    std::promise<Resultado> listo;
    auto preparado = listo.get_future();
    sesion.prepararPaso([&](Resultado resultado, std::exception_ptr error) {
        EXPECT_FALSE(error != nullptr);
        listo.set_value(resultado);
    });
    EXPECT_TRUE(static_cast<bool>(preparado.get()));

    std::promise<Resultado> alta;
    auto procesoAgregado = alta.get_future();
    sesion.agregar({1, "sesion", 32, 15, {}},
                   [&](Resultado resultado, std::exception_ptr error) {
                       EXPECT_FALSE(error != nullptr);
                       alta.set_value(resultado);
                   });
    EXPECT_TRUE(static_cast<bool>(procesoAgregado.get()));

    std::promise<EstadoSistema> leido;
    auto snapshot = leido.get_future();
    std::promise<bool> fueraUI;
    auto lecturaFueraUI = fueraUI.get_future();
    sesion.snapshot([&](EstadoSistema estado, std::exception_ptr error) {
        EXPECT_FALSE(error != nullptr);
        fueraUI.set_value(std::this_thread::get_id() != hiloUI);
        leido.set_value(std::move(estado));
    });
    EXPECT_TRUE(lecturaFueraUI.get());
    EXPECT_EQ(snapshot.get().procesos.size(), 1U);

    std::promise<ResultadoPaso> paso;
    auto ejecutado = paso.get_future();
    sesion.siguientePaso([&](ResultadoPaso resultado, std::exception_ptr error) {
        EXPECT_FALSE(error != nullptr);
        paso.set_value(resultado);
    });
    const ResultadoPaso resultadoPaso = ejecutado.get();
    EXPECT_EQ(resultadoPaso.codigo, CodigoResultado::OK);
    EXPECT_TRUE(resultadoPaso.ejecutoCpu);
    EXPECT_EQ(resultadoPaso.tiempoAvanzadoMs, 15);

    std::promise<Resultado> reiniciado;
    auto finReinicio = reiniciado.get_future();
    sesion.reiniciarParaPasos([&](Resultado resultado, std::exception_ptr error) {
        EXPECT_FALSE(error != nullptr);
        reiniciado.set_value(resultado);
    });
    EXPECT_TRUE(static_cast<bool>(finReinicio.get()));

    std::promise<EstadoSistema> leidoTrasReinicio;
    auto snapshotTrasReinicio = leidoTrasReinicio.get_future();
    sesion.snapshot([&](EstadoSistema estado, std::exception_ptr error) {
        EXPECT_FALSE(error != nullptr);
        leidoTrasReinicio.set_value(std::move(estado));
    });
    const EstadoSistema reinicio = snapshotTrasReinicio.get();
    EXPECT_EQ(reinicio.tiempoSimuladoMs, 0);
    EXPECT_EQ(reinicio.procesos.size(), 0U);
    EXPECT_EQ(reinicio.estadoEjecucion, EstadoEjecucion::PAUSADO);

    return test::finish();
}
