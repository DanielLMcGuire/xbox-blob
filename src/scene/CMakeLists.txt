add_library(scene
    ${CMAKE_CURRENT_SOURCE_DIR}/cam_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_anim.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_instances.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_texgen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_mesh_gen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_renderer.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/logo_mesh_gen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logo_renderer.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/green_fog.cpp
)

add_dependencies(scene mesh_generation)
target_include_directories(scene PRIVATE "${MESH_OUT_DIR}")
target_link_libraries(scene PRIVATE raylib)
xbb_target_embed(scene)