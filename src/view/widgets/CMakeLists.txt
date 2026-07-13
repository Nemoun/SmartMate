add_library(smartmate_widgets STATIC
    MainWindowDependencies.h
    MainWindow.h
    MainWindow.cpp
    binding/WidgetBinding.h
    settings/SettingsPage.h
    settings/SettingsPage.cpp
    theme/WidgetTheme.h
    theme/WidgetTheme.cpp
)

target_include_directories(smartmate_widgets
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(smartmate_widgets
    PUBLIC
        smartmate_common
        smartmate_viewmodel_contracts
        Qt6::Widgets
)
