# vcpkg has a config file that exports GLEW::GLEW
# GLEW has a native cmake find module that exports GLEW::glew

find_package(GLEW CONFIG QUIET)

if (GLEW_FOUND)
    message(DEBUG "Found GLEW through config file")
    add_library(GLEW::glew ALIAS GLEW::GLEW)
else()
    set(_original_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
    unset(CMAKE_MODULE_PATH)
    find_package(GLEW MODULE QUIET)
    find_package(OpenGL MODULE QUIET)
    set(CMAKE_MODULE_PATH ${_original_CMAKE_MODULE_PATH})
    if (NOT GLEW_FOUND OR NOT TARGET GLEW::glew OR 
        NOT OPENGL_FOUND OR NOT TARGET OpenGL::GL OR NOT TARGET OpenGL::GLU)
        if (Wrap_GLEW_FIND_REQUIRED)
            message(FATAL_ERROR "Could NOT find GLEW")
        endif()
    else()
        message(DEBUG "Found GLEW through native cmake module")
        target_link_libraries(GLEW::glew INTERFACE OpenGL::GL OpenGL::GLU)
    endif()
endif()
