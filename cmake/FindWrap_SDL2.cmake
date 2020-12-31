# vcpkg has a config file that export SDL2::SDL2
# otherwise we use our own find module that also exports SDL2::SDL2

find_package(SDL2 CONFIG QUIET)

if (NOT SDL2_FOUND OR NOT TARGET SDL2::SDL2)
    set(_original_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
    set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
    find_package(SDL2 MODULE QUIET)
    set(CMAKE_MODULE_PATH ${_original_CMAKE_MODULE_PATH})
    if (NOT SDL2_FOUND AND Wrap_SDL2_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find SDL2")
    else()
        message(DEBUG "Found SDL2 through custom find module")
    endif()
else()
    message(DEBUG "Found SDL2 through config file")
endif()
