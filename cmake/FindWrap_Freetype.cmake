find_package(freetype CONFIG QUIET) # this is for vcpkg support, defines freetype target

if (freetype_FOUND)
    message(DEBUG "Found Freetype through Config file")
    add_library(Freetype::Freetype ALIAS freetype)
else()
    set(_original_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
    unset(CMAKE_MODULE_PATH)
    find_package(Freetype MODULE QUIET) # cmake-native module, defines Freetype::Freetype
    set(CMAKE_MODULE_PATH ${_original_CMAKE_MODULE_PATH})
    if(NOT Freetype_FOUND AND Wrap_Freetype_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find Freetype")
    else()
        message(DEBUG "Found Freetype through find module")
    endif()
endif()


