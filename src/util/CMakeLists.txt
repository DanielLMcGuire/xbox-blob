add_library(noclip
    ${CMAKE_CURRENT_SOURCE_DIR}/noclip.cpp
)

target_link_libraries(noclip PRIVATE raylib)