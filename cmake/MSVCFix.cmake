    string(REPLACE
        " --manifests"
        ""
        CMAKE_C_LINK_EXECUTABLE
        "${CMAKE_C_LINK_EXECUTABLE}"
    )

    string(REPLACE
        " --manifests"
        ""
        CMAKE_CXX_LINK_EXECUTABLE
        "${CMAKE_CXX_LINK_EXECUTABLE}"
    )

    string(REPLACE
        " --manifests"
        ""
        CMAKE_C_CREATE_SHARED_LIBRARY
        "${CMAKE_C_CREATE_SHARED_LIBRARY}"
    )

    string(REPLACE
        " --manifests"
        ""
        CMAKE_CXX_CREATE_SHARED_LIBRARY
        "${CMAKE_CXX_CREATE_SHARED_LIBRARY}"
    )