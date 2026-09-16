cmake_minimum_required(VERSION 3.20)

project(xbox_blob_raylib CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
include("cmake/raylib.cmake")
include("cmake/sound.cmake")

add_subdirectory("thirdparty")
add_subdirectory("src")

add_executable(xbox_startup
    src/util/noclip.cpp
    src/main.cpp
)


target_link_libraries(xbox_startup PRIVATE app raylib)

target_include_directories(sos PRIVATE "${SOUND_OUT_DIR}")

if (APPLE)
    target_link_libraries(xbox_startup PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()

if (EMSCRIPTEN)
    set_target_properties(xbox_startup PROPERTIES SUFFIX ".html")

    target_compile_definitions(xbox_startup PRIVATE GRAPHICS_API_OPENGL_ES3)

    target_link_options(xbox_startup PRIVATE
        "-sUSE_GLFW=3"
        "-sINITIAL_MEMORY=67108864"
        "-sEXPORTED_RUNTIME_METHODS=ccall"
        "-sMAX_WEBGL_VERSION=2"
        "-sMIN_WEBGL_VERSION=2"
        "-sASYNCIFY"
        "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/src/main.html"
    )

    set_property(TARGET xbox_startup APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.html")
endif()