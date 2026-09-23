#include "contracts/EstadoSistema.h"
#include "contracts/IExecutionEngine.h"
#include "tests/Fixtures.h"
#include "tests/TestSupport.h"

#include <stdexcept>

namespace {

class MotorPrueba final : public IExecutionEngine {
public:
    void agregarProceso(const Proceso& proceso) override {
        if (!procesoValido(proceso)) {
            throw std::invalid_argument("Proceso invalido");
        }
        for (const Proceso& existente : estado_.procesos) {
            if (existente.pid == proceso.pid) {
                throw std::invalid_argument("PID duplicado");
            }
        }
        estado_.procesos.push_back(proceso);
    }

    void cancelarProceso(int pid) override {
        for (Proceso& proceso : estado_.procesos) {
            if (proceso.pid == pid) {
                proceso.estado = Estado::CANCEL;
            }
        }
    }

    void iniciar() override {
        estado_.estadoEjecucion = EstadoEjecucion::EJECUTANDO;
    }

    void pausar() override {
        estado_.estadoEjecucion = EstadoEjecucion::PAUSADO;
    }

    void reiniciar() override {
        estado_ = EstadoSistema{};
        estado_.quantumMs = quantumMs_;
    }

    void siguientePaso() override {
        if (estado_.estadoEjecucion != EstadoEjecucion::PAUSADO) {
            throw std::logic_error("El motor debe estar pausado");
        }
        estado_.tiempoSimulado += quantumMs_;
    }

    void configurarQuantum(std::int64_t quantumMs) override {
        if (quantumMs <= 0) {
            throw std::invalid_argument("Quantum invalido");
        }
        quantumMs_ = quantumMs;
        estado_.quantumMs = quantumMs;
    }

    std::int64_t obtenerQuantum() const override {
        return quantumMs_;
    }

    EstadoSistema obtenerEstado() const override {
        return estado_;
    }

private:
    std::int64_t quantumMs_{500};
    EstadoSistema estado_;
};

}  // namespace

int main() {
    EXPECT_EQ(estadoToString(Estado::READY), "READY");
    EXPECT_EQ(estadoToString(Estado::RUN), "RUN");
    EXPECT_EQ(estadoToString(Estado::WAIT), "WAIT");
    EXPECT_EQ(estadoToString(Estado::FINISH), "FINISH");
    EXPECT_EQ(estadoToString(Estado::CANCEL), "CANCEL");
    EXPECT_EQ(estadoToString(static_cast<Estado>(999)), "UNKNOWN");

    Proceso proceso = crearProceso(1, "Editor", 512);
    EXPECT_TRUE(procesoValido(proceso));
    EXPECT_EQ(proceso.estado, Estado::READY);
    EXPECT_EQ(proceso.prioridadBase, 5);
    EXPECT_EQ(proceso.tiempoRestante, proceso.tiempoTotal);

    proceso.prioridadBase = 1;
    proceso.prioridadActual = 10;
    EXPECT_TRUE(procesoValido(proceso));

    proceso.prioridadBase = 0;
    EXPECT_FALSE(procesoValido(proceso));
    proceso.prioridadBase = 11;
    EXPECT_FALSE(procesoValido(proceso));

    proceso = crearProceso(1, "Editor", 512);
    proceso.tiempoRestante = proceso.tiempoTotal + 1;
    EXPECT_FALSE(procesoValido(proceso));
    proceso = crearProceso(1, "", 512);
    EXPECT_FALSE(procesoValido(proceso));
    proceso = crearProceso(0, "Editor", 512);
    EXPECT_FALSE(procesoValido(proceso));
    proceso = crearProceso(1, "Editor", 0);
    EXPECT_FALSE(procesoValido(proceso));

    proceso = crearProceso(1, "Editor", 512);
    proceso.operacionIO = OperacionIO{true, 1'000, 750};
    EXPECT_TRUE(procesoValido(proceso));
    proceso.operacionIO = OperacionIO{true, 0, 750};
    EXPECT_FALSE(procesoValido(proceso));
    proceso.operacionIO = OperacionIO{true, 2'000, 750};
    EXPECT_FALSE(procesoValido(proceso));
    proceso.operacionIO = OperacionIO{true, 1'000, 0};
    EXPECT_FALSE(procesoValido(proceso));

    EXPECT_EQ(tipoEventoToString(TipoEvento::PROCESS_CREATED),
              "PROCESS_CREATED");
    EXPECT_EQ(tipoEventoToString(TipoEvento::PROCESS_ADMITTED),
              "PROCESS_ADMITTED");
    EXPECT_EQ(tipoEventoToString(TipoEvento::PRIORITY_CHANGED),
              "PRIORITY_CHANGED");
    EXPECT_EQ(tipoEventoToString(static_cast<TipoEvento>(999)), "UNKNOWN");

    EstadoSistema estado;
    EXPECT_EQ(estado.pidEjecutando, -1);
    EXPECT_EQ(estado.tiempoSimulado, 0);
    EXPECT_EQ(estado.estadoEjecucion, EstadoEjecucion::DETENIDO);
    EXPECT_EQ(estado.quantumMs, ConfiguracionPredeterminada::QUANTUM_MS);
    EXPECT_TRUE(estado.procesos.empty());
    EXPECT_TRUE(estado.eventos.empty());

    BloqueTimeline bloque{7, 500, 1'000};
    EXPECT_EQ(bloque.pid, 7);
    EXPECT_EQ(bloque.fin - bloque.inicio, 500);
    EXPECT_TRUE(bloqueTimelineValido(bloque));
    bloque.fin = bloque.inicio;
    EXPECT_FALSE(bloqueTimelineValido(bloque));

    EXPECT_EQ(ConfiguracionPredeterminada::PRIORIDAD_MINIMA, 1);
    EXPECT_EQ(ConfiguracionPredeterminada::PRIORIDAD_MAXIMA, 10);
    EXPECT_EQ(ConfiguracionPredeterminada::AGING_INTERVALO_MS, 2'000);
    EXPECT_EQ(ConfiguracionPredeterminada::AGING_INCREMENTO, 1);

    MotorPrueba motor;
    EXPECT_THROW(motor.configurarQuantum(0), std::invalid_argument);
    motor.configurarQuantum(250);
    EXPECT_EQ(motor.obtenerQuantum(), 250);
    motor.agregarProceso(crearProceso(10, "Mock", 100));
    EXPECT_THROW(motor.agregarProceso(crearProceso(10, "Duplicado", 100)),
                 std::invalid_argument);
    motor.iniciar();
    EXPECT_THROW(motor.siguientePaso(), std::logic_error);
    motor.pausar();
    motor.siguientePaso();
    EstadoSistema snapshot = motor.obtenerEstado();
    EXPECT_EQ(snapshot.estadoEjecucion, EstadoEjecucion::PAUSADO);
    EXPECT_EQ(snapshot.tiempoSimulado, 250);
    EXPECT_EQ(snapshot.quantumMs, 250);
    EXPECT_EQ(snapshot.procesos.size(), 1U);
    snapshot.procesos[0].nombre = "Copia";
    EXPECT_EQ(motor.obtenerEstado().procesos[0].nombre, "Mock");

    return test::finish();
}
