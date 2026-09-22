cmake_minimum_required(VERSION 3.20)

project(xbox_blob_raylib CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(WINDBG "Enable win32 output" OFF)

if(WIN32)
    enable_language(RC)
endif()

if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/ClangFix.cmake)
elseif(WIN32 AND MSVC)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/MSVCFix.cmake)
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
include("cmake/raylib.cmake")
include("cmake/sound.cmake")
include("cmake/mesh.cmake")

add_subdirectory("thirdparty")
add_subdirectory("src")

if(WINDBG)
    add_executable(xbox_startup
        src/main.rc
        src/main.cpp
    )
else()
    if (WIN32)
        add_compile_definitions(XBS_WIN32_DESKTOP)
    endif()
    add_executable(xbox_startup WIN32
        src/main.rc
        src/main.cpp
    )
endif()

if(WIN32)
    if(MSVC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_link_options(xbox_startup PRIVATE "-Xlinker" "/ENTRY:mainCRTStartup")
    endif()
endif()

target_link_libraries(xbox_startup PRIVATE app raylib)

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