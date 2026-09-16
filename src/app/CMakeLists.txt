add_library(app
    ${CMAKE_CURRENT_SOURCE_DIR}/app.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/capture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/interactive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui.cpp
)

target_link_libraries(app PRIVATE raylib rlimgui scene sos blob)