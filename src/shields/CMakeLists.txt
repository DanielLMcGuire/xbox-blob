add_library(shields
    ${CMAKE_CURRENT_SOURCE_DIR}/shield_envmap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shield_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shield_mesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shield_motion.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/shield_envmap.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shield_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shield_mesh.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shield_motion.h  
)

target_link_libraries(shields PRIVATE raylib scene blob)
xbb_target_embed(shields)