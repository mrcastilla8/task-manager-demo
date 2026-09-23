#pragma once

#include <string>

enum class Estado {
    READY,
    RUN,
    WAIT,
    FINISH,
    CANCEL
};

inline std::string estadoToString(Estado estado) {
    switch (estado) {
        case Estado::READY:
            return "READY";
        case Estado::RUN:
            return "RUN";
        case Estado::WAIT:
            return "WAIT";
        case Estado::FINISH:
            return "FINISH";
        case Estado::CANCEL:
            return "CANCEL";
    }

    return "UNKNOWN";
}
