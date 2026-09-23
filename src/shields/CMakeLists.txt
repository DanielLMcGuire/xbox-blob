add_library(shields
    ${CMAKE_CURRENT_SOURCE_DIR}/shield_envmap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shield_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shield_mesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shield_motion.cpp
)

target_link_libraries(shields PRIVATE raylib scene blob)
xbb_target_embed(shields)