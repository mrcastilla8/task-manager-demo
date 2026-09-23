cmake_minimum_required(VERSION 3.20)

project(ConcurrentSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SIMULATOR_WARNINGS_AS_ERRORS
       "Tratar las advertencias del compilador como errores" ON)

add_library(simulator_core
    core/ColaProcesos.cpp
    core/Memoria.cpp
)

target_include_directories(simulator_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(simulator_scheduler
    scheduler/Scheduler.cpp
)

target_include_directories(simulator_scheduler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(simulator_core PRIVATE /W4 /permissive-)
    target_compile_options(simulator_scheduler PRIVATE /W4 /permissive-)
    if(SIMULATOR_WARNINGS_AS_ERRORS)
        target_compile_options(simulator_core PRIVATE /WX)
        target_compile_options(simulator_scheduler PRIVATE /WX)
    endif()
else()
    target_compile_options(simulator_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(simulator_scheduler PRIVATE -Wall -Wextra -Wpedantic)
    if(SIMULATOR_WARNINGS_AS_ERRORS)
        target_compile_options(simulator_core PRIVATE -Werror)
        target_compile_options(simulator_scheduler PRIVATE -Werror)
    endif()
endif()

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

