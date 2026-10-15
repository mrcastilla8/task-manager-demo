function(add_simulator_test test_name)
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE simulator_core)

    if(MSVC)
        target_compile_options(${test_name} PRIVATE /W4 /permissive-)
        if(SIMULATOR_WARNINGS_AS_ERRORS)
            target_compile_options(${test_name} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${test_name} PRIVATE -Wall -Wextra -Wpedantic)
        if(SIMULATOR_WARNINGS_AS_ERRORS)
            target_compile_options(${test_name} PRIVATE -Werror)
        endif()
    endif()

    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_simulator_test(test_proceso)
add_simulator_test(test_cola)
add_simulator_test(test_memoria)
add_simulator_test(test_integracion)

# Presentación sin Qt: comprueba gaps, entradas y fixtures sin migrar core.
add_executable(test_gui_presentation ../gui/test_presentation.cpp
    ../gui/Presentation.cpp ../gui/PreviewFrames.cpp)
target_include_directories(test_gui_presentation PRIVATE ${PROJECT_SOURCE_DIR})
add_test(NAME test_gui_presentation COMMAND test_gui_presentation)
