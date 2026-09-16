set(APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/app.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/capture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/interactive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui.cpp
)

if(WIN32)
    list(APPEND APP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/win32_window.cpp
    )
endif()

add_library(app ${APP_SOURCES})

target_link_libraries(app PRIVATE raylib rlimgui scene sos blob)