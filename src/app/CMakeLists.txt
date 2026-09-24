set(APP_LIBS raylib rlimgui scene shields sos sos_boot blob noclip)

if(WIN32)
    list(APPEND APP_LIBS windowstuff)
endif()

add_library(app 
    ${CMAKE_CURRENT_SOURCE_DIR}/app.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/capture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/interactive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/font.cpp
)

if(NOT HAVE_CXX_EMBED)
    add_dependencies(app font_file)
endif()

target_link_libraries(app PRIVATE ${APP_LIBS})
xbb_target_embed(app)
