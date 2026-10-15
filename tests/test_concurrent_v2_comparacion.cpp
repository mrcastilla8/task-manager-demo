// Comparacion del motor concurrente con el motor logico de Aldana (DP-11b):
// mismas entradas, mismos pasos, mismo resultado observable. Este es el unico
// archivo de pruebas del motor concurrente que usa SimulationEngineV2, y solo
// como referencia para comparar.
#include "simulation/SimulationEngineV2.h"
#include "tests/ConcurrentV2Support.h"
#include "tests/TestSupport.h"

#include <cstdint>
#include <iostream>
#include <string>

using namespace ConcurrentSimulator::v2;
using namespace ConcurrentSimulator::v2::testing;

namespace {

// Aplica cada operacion a los dos motores y exige el mismo codigo de retorno y
// el mismo snapshot completo (reloj, estados, prioridades, tickets, eventos,
// timeline, cola y memoria).
class Pareja {
public:
    Pareja(std::string escenario, std::int64_t memoria)
        : escenario_(std::move(escenario)),
          logico_(Configuracion{memoria}),
          concurrente_(Configuracion{memoria}) {}

    void alta(const AltaProceso& a) {
        igualResultado("alta", logico_.agregarProceso(a),
                       concurrente_.agregarProceso(a));
    }
    void cancelar(int pid) {
        igualResultado("cancelar", logico_.cancelarProceso(pid),
                       concurrente_.cancelarProceso(pid));
    }
    void iniciar() {
        igualResultado("iniciar", logico_.iniciar(), concurrente_.iniciar());
    }
    void pausar() {
        igualResultado("pausar", logico_.pausar(), concurrente_.pausar());
    }
    void reiniciar() {
        igualResultado("reiniciar", logico_.reiniciar(),
                       concurrente_.reiniciar());
    }
    ResultadoPaso paso() {
        const ResultadoPaso esperado = logico_.siguientePaso();
        const ResultadoPaso real = concurrente_.siguientePaso();
        ++operaciones_;
        if (esperado.codigo != real.codigo || esperado.avanzo != real.avanzo ||
            esperado.ejecutoCpu != real.ejecutoCpu ||
            esperado.tiempoAvanzadoMs != real.tiempoAvanzadoMs) {
            registrar("siguientePaso: resultado distinto");
        }
        comparar("siguientePaso");
        return real;
    }
    // Deja ambos motores en PAUSADO (iniciar + pausar) si es posible.
    void prepararPaso() {
        iniciar();
        pausar();
    }
    // Avanza hasta FINALIZADO o hasta agotar el limite de pasos.
    int correrHastaFinalizar(int limite = 500) {
        int pasos = 0;
        while (pasos < limite &&
               concurrente_.obtenerEstado().estadoEjecucion !=
                   EstadoEjecucion::FINALIZADO) {
            const ResultadoPaso r = paso();
            ++pasos;
            if (r.codigo != CodigoResultado::OK) break;
        }
        return pasos;
    }
    EstadoSistema estado() const { return concurrente_.obtenerEstado(); }
    int diferencias() const { return diferencias_; }
    int operaciones() const { return operaciones_; }

private:
    template <typename R>
    void igualResultado(const char* operacion, const R& esperado, const R& real) {
        ++operaciones_;
        if (esperado.codigo != real.codigo) {
            registrar(std::string(operacion) + ": codigo distinto");
        }
        comparar(operacion);
    }
    void comparar(const char* operacion) {
        const std::string motivo =
            diferencia(logico_.obtenerEstado(), concurrente_.obtenerEstado());
        if (!motivo.empty()) {
            registrar(std::string(operacion) + ": " + motivo);
        }
        if (invariantesVioladas(concurrente_.obtenerEstado()) != 0) {
            registrar(std::string(operacion) + ": invariantes violadas");
        }
    }
    void registrar(const std::string& detalle) {
        ++diferencias_;
        std::cerr << "[" << escenario_ << " op " << operaciones_ << "] "
                  << detalle << '\n';
    }

    std::string escenario_;
    SimulationEngineV2 logico_;
    ConcurrentEngineV2 concurrente_;
    int diferencias_{0};
    int operaciones_{0};
};

// Generador pseudoaleatorio determinista (no depende de la plataforma).
class Lcg {
public:
    explicit Lcg(std::uint64_t semilla) : estado_(semilla) {}
    std::uint64_t siguiente(std::uint64_t limite) {
        estado_ = estado_ * 6364136223846793005ULL + 1442695040888963407ULL;
        return (estado_ >> 33) % limite;
    }

private:
    std::uint64_t estado_;
};

}  // namespace

int main() {
    // Escenario 1 (casos aprobados 1 y 4): tamanos de frontera y cuotas 10/20/30.
    {
        Pareja p("clasificacion", 100000);
        const std::int64_t tamanios[] = {12, 39, 40, 67, 68, 96};
        int pid = 1;
        for (const std::int64_t t : tamanios) {
            p.alta(alta(pid++, t, 55));
        }
        p.prepararPaso();
        p.correrHastaFinalizar();
        EXPECT_EQ(p.diferencias(), 0);
        EXPECT_EQ(p.estado().estadoEjecucion, EstadoEjecucion::FINALIZADO);
    }

    // Escenario 2: E/S, WAIT y retorno (el del integrador, ampliado).
    {
        Pareja p("wait", 250);
        p.alta(alta(1, 12, 30, {true, 10, 5}));
        p.alta(alta(2, 40, 50));
        p.alta(alta(3, 96, 5));
        p.prepararPaso();
        p.correrHastaFinalizar();
        EXPECT_EQ(p.diferencias(), 0);
    }

    // Escenario 3 (caso aprobado 9): salto idle sin bloque en el timeline.
    {
        Pareja p("idle", 1000);
        p.alta(alta(1, 50, 20, {true, 8, 10}));
        p.prepararPaso();
        p.correrHastaFinalizar();
        EXPECT_EQ(p.diferencias(), 0);
        const EstadoSistema s = p.estado();
        EXPECT_EQ(s.timeline.size(), 2U);
        EXPECT_EQ(s.timeline[0].inicioMs(), 0);
        EXPECT_EQ(s.timeline[0].finMs(), 8);
        EXPECT_EQ(s.timeline[1].inicioMs(), 18);  // hueco idle [8, 18)
    }

    // Escenario 4 (caso aprobado 3): altas durante la ejecucion y reclasificacion.
    {
        Pareja p("alta dinamica", 100000);
        p.alta(alta(1, 12, 90));
        p.alta(alta(2, 96, 90));
        p.prepararPaso();
        for (int i = 0; i < 3; ++i) p.paso();
        p.alta(alta(3, 1000, 60));  // cambia las bandas a mitad de ejecucion
        for (int i = 0; i < 4; ++i) p.paso();
        p.alta(alta(4, 5, 40));
        p.correrHastaFinalizar();
        EXPECT_EQ(p.diferencias(), 0);
    }

    // Escenario 5 (caso aprobado 7): cancelacion de READY y de WAIT.
    {
        Pareja p("cancelacion", 100000);
        p.alta(alta(1, 96, 100));
        p.alta(alta(2, 40, 100, {true, 20, 100}));
        p.alta(alta(3, 12, 100));
        p.prepararPaso();
        p.cancelar(3);          // READY
        p.paso();               // P1 corre
        p.cancelar(1);          // READY tras reingreso
        p.paso();               // P2 corre y entra en WAIT
        p.cancelar(2);          // WAIT
        p.cancelar(2);          // ya cancelado
        p.cancelar(99);         // inexistente
        p.paso();
        EXPECT_EQ(p.diferencias(), 0);
        EXPECT_EQ(p.estado().estadoEjecucion, EstadoEjecucion::FINALIZADO);
    }

    // Escenario 6: rechazos de alta (memoria, duplicado, invalida).
    {
        Pareja p("rechazos", 100);
        p.alta(alta(1, 60, 50));
        p.alta(alta(2, 60, 50));   // sin memoria
        p.alta(alta(1, 10, 50));   // duplicado
        p.alta(alta(0, 10, 50));   // invalida
        p.alta(alta(3, 10, 0));    // CPU invalida
        p.alta(alta(4, 10, 50, {true, 60, 5}));  // E/S invalida
        p.prepararPaso();
        p.correrHastaFinalizar();
        EXPECT_EQ(p.diferencias(), 0);
    }

    // Escenario 7: reinicio y reutilizacion de la sesion.
    {
        Pareja p("reinicio", 5000);
        p.alta(alta(1, 30, 80));
        p.alta(alta(2, 60, 80, {true, 10, 15}));
        p.prepararPaso();
        for (int i = 0; i < 4; ++i) p.paso();
        p.reiniciar();
        p.alta(alta(1, 30, 40));
        p.prepararPaso();
        p.correrHastaFinalizar();
        EXPECT_EQ(p.diferencias(), 0);
    }

    // Escenario 8 (caso aprobado 6): varios WAIT que despiertan en el mismo
    // instante. El orden exacto esta pendiente de ratificacion; aqui solo se
    // exige que ambos motores coincidan.
    {
        Pareja p("wait simultaneo", 100000);
        p.alta(alta(1, 50, 60, {true, 10, 40}));
        p.alta(alta(2, 50, 60, {true, 10, 40}));
        p.alta(alta(3, 50, 60, {true, 10, 40}));
        p.prepararPaso();
        p.correrHastaFinalizar();
        EXPECT_EQ(p.diferencias(), 0);
    }

    // Escenario 9: operaciones aleatorias deterministas (alta, cancelar, estado
    // de ejecucion y pasos) sobre ambos motores. Cualquier divergencia de codigo
    // de retorno o de snapshot en cualquier momento falla la prueba.
    for (std::uint64_t semilla = 1; semilla <= 12; ++semilla) {
        Lcg azar(semilla * 7919);
        Pareja p("fuzz semilla " + std::to_string(semilla), 600);
        int siguientePid = 1;
        for (int operacion = 0; operacion < 140; ++operacion) {
            switch (azar.siguiente(10)) {
                case 0:
                case 1: {
                    const std::int64_t total = 1 + static_cast<std::int64_t>(azar.siguiente(90));
                    OperacionIO io;
                    if (total > 2 && azar.siguiente(3) == 0) {
                        io = {true, 1 + static_cast<std::int64_t>(azar.siguiente(
                                          static_cast<std::uint64_t>(total - 1))),
                              1 + static_cast<std::int64_t>(azar.siguiente(60))};
                    }
                    p.alta(alta(siguientePid++,
                                1 + static_cast<std::int64_t>(azar.siguiente(300)),
                                total, io));
                    break;
                }
                case 2:
                    p.cancelar(1 + static_cast<int>(azar.siguiente(
                                       static_cast<std::uint64_t>(siguientePid + 1))));
                    break;
                case 3:
                    p.iniciar();
                    break;
                case 4:
                    p.pausar();
                    break;
                case 5:
                    if (azar.siguiente(12) == 0) p.reiniciar();
                    break;
                default:
                    p.paso();
                    break;
            }
        }
        EXPECT_EQ(p.diferencias(), 0);
        EXPECT_TRUE(p.operaciones() > 100);
    }

    return test::finish();
}
