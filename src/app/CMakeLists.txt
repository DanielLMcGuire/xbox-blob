set(APP_LIBS raylib rlimgui scene shields sos sos_boot blob noclip)

if(WIN32)
    list(APPEND APP_LIBS windowstuff)
endif()

if(NOT HAVE_CXX_EMBED)
    list(APPEND APP_LIBS font_file)
endif()

add_library(app 
    ${CMAKE_CURRENT_SOURCE_DIR}/app.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/capture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/interactive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/font.cpp
)

target_link_libraries(app PRIVATE ${APP_LIBS})
xbb_target_embed(app)
