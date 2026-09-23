if(NOT HAVE_CXX_EMBED)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/assets")
    configure_file(
        "${CMAKE_SOURCE_DIR}/assets/xbox.ttf"
        "${CMAKE_BINARY_DIR}/assets/xbox.ttf"
        COPYONLY
    )

add_custom_target(font_file DEPENDS "${CMAKE_BINARY_DIR}/assets/xbox.ttf")
endif()