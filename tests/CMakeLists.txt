qt_add_executable(tst_task_service
    model/tst_TaskService.cpp
    fakes/FakeTaskRepository.h
)

target_include_directories(tst_task_service
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(tst_task_service
    PRIVATE
        smartmate_model
        Qt6::Test
)

add_test(NAME model.task_service COMMAND tst_task_service)

add_test(
    NAME view.qml_bootstrap
    COMMAND SmartMate --smoke-test -platform offscreen
)
set_tests_properties(view.qml_bootstrap PROPERTIES TIMEOUT 10)

get_target_property(qmake_executable Qt6::qmake IMPORTED_LOCATION)
get_filename_component(qt_runtime_directory "${qmake_executable}" DIRECTORY)
get_filename_component(compiler_runtime_directory "${CMAKE_CXX_COMPILER}" DIRECTORY)
set(runtime_path_modifications
    "PATH=path_list_prepend:${qt_runtime_directory}"
    "PATH=path_list_prepend:${compiler_runtime_directory}"
)

set_tests_properties(
    model.task_service
    view.qml_bootstrap
    PROPERTIES
        ENVIRONMENT_MODIFICATION
            "${runtime_path_modifications}"
)

add_test(
    NAME architecture.mvvm_boundaries
    COMMAND
        ${CMAKE_COMMAND}
        -DROOT_DIR=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/architecture/check_mvvm_boundaries.cmake
)
