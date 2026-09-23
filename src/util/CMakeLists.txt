add_library(noclip
    ${CMAKE_CURRENT_SOURCE_DIR}/noclip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/noclip.h
)

target_link_libraries(noclip PRIVATE raylib)

if(WIN32)
    add_library(windowstuff
        ${CMAKE_CURRENT_SOURCE_DIR}/win32_window.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/win32_window.h
    )
    target_link_libraries(windowstuff PRIVATE raylib)
endif()