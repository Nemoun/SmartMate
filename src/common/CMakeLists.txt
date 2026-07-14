add_library(smartmate_common STATIC
    presentation/UiNotification.h
    presentation/UiNotification.cpp
)

target_include_directories(smartmate_common
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(smartmate_common
    PUBLIC
        Qt6::Core
)
