# vcpkg has an sdl2-mixer config file that exports SDL2::SDL2_mixer
# otherwise we use our own find module that exports SDL2::SDl2_mixer

find_package(sdl2-mixer CONFIG QUIET)

if (sdl2-mixer_FOUND)
    message(DEBUG "found SDL2_mixer through config file")
else()
    set(_original_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
    set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
    find_package(SDL2_mixer MODULE QUIET) # find module, defines SDL2::SDL2_mixer
    set(CMAKE_MODULE_PATH ${_original_CMAKE_MODULE_PATH})
    if (NOT SDL2_MIXER_FOUND AND Wrap_SDL2_mixer_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find SDL2_mixer")
    else()
        message(DEBUG "found SDL2_mixer through custom find module")
    endif()
endif()
