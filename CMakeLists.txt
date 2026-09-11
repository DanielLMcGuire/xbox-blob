cmake_minimum_required(VERSION 3.16)

project(xbox_blob_raylib CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
    GIT_SHALLOW TRUE
)

set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
set(SUPPORT_FILEFORMAT_TGA 1 CACHE STRING "" FORCE)
set(SUPPORT_CUSTOM_FRAME_CONTROL OFF CACHE BOOL "" FORCE)
set(SUPPORT_BUSY_WAIT_LOOP OFF CACHE BOOL "" FORCE)
set(SUPPORT_PARTIALBUSY_WAIT_LOOP ON CACHE BOOL "" FORCE)
set(SUPPORT_WINMM_HIGHRES_TIMER ON CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(raylib)

add_executable(xbox_blob
    src/main.cpp
    src/blob.cpp
)

target_link_libraries(xbox_blob PRIVATE raylib)

if (APPLE)
    target_link_libraries(xbox_blob PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()

file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/shaders/*"
)

add_custom_target(xbox_blob_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/src/shaders"
        "${CMAKE_BINARY_DIR}/shaders"
    DEPENDS ${SHADER_FILES}
)

add_dependencies(xbox_blob xbox_blob_shaders)

add_custom_command(
    TARGET xbox_blob
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:xbox_blob>/shaders"
)
