add_library(smartmate_viewmodel_contracts STATIC
    AppearanceSettingsContract.h
    TaskCategoryContract.h
    TaskDependencyContract.h
    TaskEditorContract.h
    TaskFocusContract.h
    TaskDetailsContract.h
    TaskGraphContract.h
    TaskListContract.h
    ViewModelContracts.cpp
)

target_include_directories(smartmate_viewmodel_contracts
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(smartmate_viewmodel_contracts
    PUBLIC
        smartmate_common
        Qt6::Core
)

# QML 迁移期的 foreign wrapper 需要消费这份元对象 JSON，Contracts 本身仍不链接 QML。
qt_extract_metatypes(smartmate_viewmodel_contracts)
