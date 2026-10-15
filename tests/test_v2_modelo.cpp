#include "core/v2/Proceso.h"
#include "tests/TestSupport.h"

#include <cstdint>
#include <limits>
#include <vector>

using namespace ConcurrentSimulator::v2;

namespace {

AltaProceso altaValida() { return {1, "proceso", 32, 100, {}}; }

Proceso proceso(int pid, std::int64_t tamanio, Estado estado = Estado::READY) {
    AltaProceso alta{pid, "p" + std::to_string(pid), tamanio, 100, {}};
    Proceso result = construirProcesoInicial(alta);
    result.estado = estado;
    return result;
}

}  // namespace

int main() {
    AltaProceso alta = altaValida();
    EXPECT_TRUE(validarAltaProceso(alta));
    const Proceso inicial = construirProcesoInicial(alta);
    EXPECT_EQ(inicial.estado, Estado::READY);
    EXPECT_EQ(inicial.tiempoCpuTotalMs, 100);
    EXPECT_EQ(inicial.tiempoCpuRestanteMs, 100);
    EXPECT_EQ(inicial.tiempoCpuConsumidoMs, 0);
    EXPECT_EQ(inicial.prioridad, Prioridad::P3);
    EXPECT_TRUE(cpuConsistente(inicial));

    alta.pid = 0;
    EXPECT_FALSE(validarAltaProceso(alta));
    alta = altaValida();
    alta.nombre.clear();
    EXPECT_FALSE(validarAltaProceso(alta));
    alta = altaValida();
    alta.tamanioBytes = 0;
    EXPECT_FALSE(validarAltaProceso(alta));
    alta = altaValida();
    alta.tiempoCpuTotalMs = 0;
    EXPECT_FALSE(validarAltaProceso(alta));
    EXPECT_THROW(construirProcesoInicial(alta), std::invalid_argument);

    alta = altaValida();
    alta.operacionIO = {true, 25, 10};
    EXPECT_TRUE(validarAltaProceso(alta));
    alta.operacionIO.despuesDeCpuMs = 0;
    EXPECT_FALSE(validarAltaProceso(alta));
    alta.operacionIO = {true, 100, 10};
    EXPECT_FALSE(validarAltaProceso(alta));
    alta.operacionIO = {true, 25, 0};
    EXPECT_FALSE(validarAltaProceso(alta));

    Proceso cpu = construirProcesoInicial(altaValida());
    cpu.tiempoCpuConsumidoMs = 40;
    cpu.tiempoCpuRestanteMs = 60;
    EXPECT_TRUE(cpuConsistente(cpu));
    cpu.tiempoCpuRestanteMs = 61;
    EXPECT_FALSE(cpuConsistente(cpu));
    cpu.tiempoCpuConsumidoMs = std::numeric_limits<std::int64_t>::max();
    EXPECT_FALSE(cpuConsistente(cpu));

    EXPECT_EQ(cuotaMs(Prioridad::P1), 10);
    EXPECT_EQ(cuotaMs(Prioridad::P2), 20);
    EXPECT_EQ(cuotaMs(Prioridad::P3), 30);

    std::vector<Proceso> vacios{proceso(1, 12, Estado::FINISH),
                                proceso(2, 96, Estado::CANCEL)};
    vacios[0].prioridad = Prioridad::P1;
    vacios[1].prioridad = Prioridad::P2;
    EXPECT_TRUE(clasificarPrioridades(vacios));
    EXPECT_EQ(vacios[0].prioridad, Prioridad::P1);
    EXPECT_EQ(vacios[1].prioridad, Prioridad::P2);

    std::vector<Proceso> bandas{proceso(1, 12), proceso(2, 39),
                                proceso(3, 40), proceso(4, 67),
                                proceso(5, 68), proceso(6, 96)};
    EXPECT_TRUE(clasificarPrioridades(bandas));
    EXPECT_EQ(bandas[0].prioridad, Prioridad::P1);
    EXPECT_EQ(bandas[1].prioridad, Prioridad::P1);
    EXPECT_EQ(bandas[2].prioridad, Prioridad::P2);
    EXPECT_EQ(bandas[3].prioridad, Prioridad::P2);
    EXPECT_EQ(bandas[4].prioridad, Prioridad::P3);
    EXPECT_EQ(bandas[5].prioridad, Prioridad::P3);

    std::vector<Proceso> estados{proceso(1, 10, Estado::READY),
                                 proceso(2, 20, Estado::RUN),
                                 proceso(3, 30, Estado::WAIT),
                                 proceso(4, 100, Estado::FINISH),
                                 proceso(5, 200, Estado::CANCEL)};
    EXPECT_TRUE(clasificarPrioridades(estados));
    EXPECT_EQ(estados[0].prioridad, Prioridad::P1);
    EXPECT_EQ(estados[1].prioridad, Prioridad::P2);
    EXPECT_EQ(estados[2].prioridad, Prioridad::P3);
    EXPECT_EQ(estados[3].prioridad, Prioridad::P3);
    EXPECT_EQ(estados[4].prioridad, Prioridad::P3);

    estados.push_back(proceso(6, 1000));
    EXPECT_TRUE(clasificarPrioridades(estados));
    EXPECT_EQ(estados[0].prioridad, Prioridad::P1);
    EXPECT_EQ(estados[2].prioridad, Prioridad::P1);
    EXPECT_EQ(estados[5].prioridad, Prioridad::P3);

    std::vector<Proceso> fallida{proceso(20, 10), proceso(21, 10)};
    fallida[1].tamanioBytes = 0;
    fallida[0].prioridad = Prioridad::P3;
    fallida[1].prioridad = Prioridad::P1;
    EXPECT_FALSE(clasificarPrioridades(fallida));
    EXPECT_EQ(fallida[0].prioridad, Prioridad::P3);
    EXPECT_EQ(fallida[1].prioridad, Prioridad::P1);

    EXPECT_EQ(clasificarTamano(5, 5, 5), Prioridad::P3);
    std::vector<Proceso> solo{proceso(7, 5)};
    EXPECT_TRUE(clasificarPrioridades(solo));
    EXPECT_EQ(solo[0].prioridad, Prioridad::P3);
    std::vector<Proceso> iguales{proceso(8, 5), proceso(9, 5)};
    EXPECT_TRUE(clasificarPrioridades(iguales));
    EXPECT_EQ(iguales[0].prioridad, Prioridad::P3);
    EXPECT_EQ(iguales[1].prioridad, Prioridad::P3);
    const auto maximo = (std::numeric_limits<std::int64_t>::max)();
    EXPECT_EQ(clasificarTamano(1, 1, maximo), Prioridad::P1);
    EXPECT_EQ(clasificarTamano(maximo, 1, maximo), Prioridad::P3);
    const std::int64_t diferencia = maximo - 1;
    EXPECT_EQ(clasificarTamano(1 + (diferencia - 1) / 3, 1, maximo),
              Prioridad::P1);
    EXPECT_EQ(clasificarTamano(1 + (diferencia - 1) / 3 + 1, 1, maximo),
              Prioridad::P2);

    return test::finish();
}
