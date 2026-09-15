add_library(rlimgui STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/rLImGui.cpp
)

target_link_libraries(rlimgui PRIVATE raylib)
target_link_libraries(rlimgui PUBLIC imgui)
target_include_directories(rlimgui PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)