add_library(blob
    ${CMAKE_CURRENT_SOURCE_DIR}/blob.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/blob_bump.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bloblet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/blob_texgen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/intensity_driver.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/blob.h
    ${CMAKE_CURRENT_SOURCE_DIR}/blob_bump.h
    ${CMAKE_CURRENT_SOURCE_DIR}/blob_math.h
    ${CMAKE_CURRENT_SOURCE_DIR}/bloblet.h
    ${CMAKE_CURRENT_SOURCE_DIR}/intensity_driver.h
)

target_link_libraries(blob PRIVATE raylib)
xbb_target_embed(blob)