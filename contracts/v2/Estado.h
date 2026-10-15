#pragma once

namespace ConcurrentSimulator::v2 {

enum class Estado { READY, RUN, WAIT, FINISH, CANCEL };
enum class EstadoEjecucion { DETENIDO, EJECUTANDO, PAUSADO, FINALIZADO };

}  // namespace ConcurrentSimulator::v2
