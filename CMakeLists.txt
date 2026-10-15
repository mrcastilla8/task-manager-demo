cmake_minimum_required(VERSION 3.20)

project(ConcurrentSimulator LANGUAGES CXX)

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SIMULATOR_WARNINGS_AS_ERRORS
       "Tratar las advertencias del compilador como errores" ON)

add_library(simulator_core
    core/ColaProcesos.cpp
    core/Memoria.cpp
)

add_library(simulator_core_v2
    core/v2/Proceso.cpp
    core/v2/ListaProcesos.cpp
    core/v2/Memoria.cpp
    core/v2/AdmisorProcesos.cpp
)

target_include_directories(simulator_core_v2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(simulator_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(simulator_scheduler
    scheduler/Scheduler.cpp
    scheduler/SchedulerV2.cpp
)

target_include_directories(simulator_scheduler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(simulator_simulation
    simulation/SimulationClockV2.cpp
    simulation/SimulationEngineV2.cpp
)
target_include_directories(simulator_simulation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(simulator_simulation PUBLIC simulator_core_v2 simulator_scheduler)

add_library(simulator_concurrent_v2
    concurrent/ProcessThread.cpp
    concurrent/ConcurrentEngineV2.cpp
)
target_include_directories(simulator_concurrent_v2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(simulator_concurrent_v2 PUBLIC simulator_simulation Threads::Threads)


if(MSVC)
    target_compile_options(simulator_core PRIVATE /W4 /permissive-)
    target_compile_options(simulator_scheduler PRIVATE /W4 /permissive-)
    target_compile_options(simulator_simulation PRIVATE /W4 /permissive-)
    target_compile_options(simulator_concurrent_v2 PRIVATE /W4 /permissive-)
    if(SIMULATOR_WARNINGS_AS_ERRORS)
        target_compile_options(simulator_core PRIVATE /WX)
        target_compile_options(simulator_scheduler PRIVATE /WX)
        target_compile_options(simulator_simulation PRIVATE /WX)
        target_compile_options(simulator_concurrent_v2 PRIVATE /WX)
    endif()
    target_compile_options(simulator_core_v2 PRIVATE /W4 /permissive-)
    if(SIMULATOR_WARNINGS_AS_ERRORS)
        target_compile_options(simulator_core_v2 PRIVATE /WX)
    endif()
else()
    target_compile_options(simulator_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(simulator_scheduler PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(simulator_simulation PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(simulator_concurrent_v2 PRIVATE -Wall -Wextra -Wpedantic)
    if(SIMULATOR_WARNINGS_AS_ERRORS)
        target_compile_options(simulator_core PRIVATE -Werror)
        target_compile_options(simulator_scheduler PRIVATE -Werror)
        target_compile_options(simulator_simulation PRIVATE -Werror)
        target_compile_options(simulator_concurrent_v2 PRIVATE -Werror)
    endif()
    target_compile_options(simulator_core_v2 PRIVATE -Wall -Wextra -Wpedantic)
    if(SIMULATOR_WARNINGS_AS_ERRORS)
        target_compile_options(simulator_core_v2 PRIVATE -Werror)
    endif()
endif()

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

