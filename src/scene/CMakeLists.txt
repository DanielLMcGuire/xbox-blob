add_library(scene
    ${CMAKE_CURRENT_SOURCE_DIR}/cam_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_anim.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_instances.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_texgen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_mesh_gen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_renderer.cpp
)

target_link_libraries(scene PRIVATE raylib)