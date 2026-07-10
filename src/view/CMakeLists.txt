set_source_files_properties(qml/Main.qml
    PROPERTIES
        QT_RESOURCE_ALIAS Main.qml
)

qt_add_qml_module(smartmate_ui
    STATIC
    URI SmartMate.View
    VERSION 1.0
    QML_FILES
        qml/Main.qml
    IMPORTS
        TARGET smartmate_viewmodel
    DEPENDENCIES
        QtQuick
        QtQuick.Controls
)

target_link_libraries(smartmate_ui
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
)
