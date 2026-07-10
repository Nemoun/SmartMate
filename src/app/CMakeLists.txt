qt_add_executable(SmartMate WIN32
    main.cpp
    AppBootstrapper.h
    AppBootstrapper.cpp
)

target_link_libraries(SmartMate
    PRIVATE
        smartmate_model
        smartmate_viewmodel
        smartmate_viewmodelplugin
        smartmate_ui
        smartmate_uiplugin
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
)

qt_import_qml_plugins(SmartMate)
