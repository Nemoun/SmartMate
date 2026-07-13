qt_add_qml_module(smartmate_viewmodel_qml
    STATIC
    URI SmartMate.ViewModel
    VERSION 1.0
    SOURCES
        ViewModelQmlForeignTypes.h
)

target_link_libraries(smartmate_viewmodel_qml
    PUBLIC
        smartmate_viewmodel
        Qt6::Core
        Qt6::QmlIntegration
)
