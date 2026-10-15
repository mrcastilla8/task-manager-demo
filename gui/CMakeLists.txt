find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

add_library(simulator_gui
    Presentation.cpp PreviewFrames.cpp EngineSession.cpp SnapshotTableModel.cpp
    MainWindow.cpp ProcessTable.cpp QueueWidget.cpp CPUWidget.cpp
    MemoryWidget.cpp TimelineWidget.cpp EventLogWidget.cpp NewProcessDialog.cpp
)
target_include_directories(simulator_gui PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(simulator_gui PUBLIC Qt6::Widgets)
if(MSVC)
    target_compile_options(simulator_gui PRIVATE /W4 /permissive- /utf-8)
    if(SIMULATOR_WARNINGS_AS_ERRORS)
        target_compile_options(simulator_gui PRIVATE /WX)
    endif()
else()
    target_compile_options(simulator_gui PRIVATE -Wall -Wextra -Wpedantic)
    if(SIMULATOR_WARNINGS_AS_ERRORS)
        target_compile_options(simulator_gui PRIVATE -Werror)
    endif()
endif()

add_executable(simulator_gui_preview WIN32 ../main.cpp)
target_link_libraries(simulator_gui_preview PRIVATE simulator_gui)

if(BUILD_TESTING)
    find_package(Qt6 6.2 REQUIRED COMPONENTS Test)
    add_executable(test_gui test_gui.cpp)
    target_link_libraries(test_gui PRIVATE simulator_gui Qt6::Test)
    add_test(NAME test_gui COMMAND test_gui)
    set_tests_properties(test_gui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(test_gui_models test_gui_models.cpp)
    target_link_libraries(test_gui_models PRIVATE simulator_gui Qt6::Test)
    add_test(NAME test_gui_models COMMAND test_gui_models)
    set_tests_properties(test_gui_models PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
