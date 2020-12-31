cmake_minimum_required(VERSION 3.15.7)

project(sgg VERSION 1.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(Wrap_GLEW MODULE REQUIRED)
find_package(Wrap_glm MODULE REQUIRED)
find_package(Wrap_SDL2 MODULE REQUIRED)
find_package(Wrap_Freetype MODULE REQUIRED) 
find_package(Wrap_SDL2_mixer MODULE REQUIRED)

add_library(sgg
    sgg/audio.cpp
    sgg/AudioManager.cpp
    sgg/fonts.cpp
    sgg/GLbackend.cpp
    sgg/graphics.cpp
    sgg/lodepng.cpp
    sgg/shader.cpp
    sgg/texture.cpp
)

target_include_directories(sgg
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(sgg
    PUBLIC
    $<TARGET_NAME_IF_EXISTS:glm::glm>
    $<TARGET_NAME_IF_EXISTS:GLEW::glew>
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2>
    $<TARGET_NAME_IF_EXISTS:Freetype::Freetype>
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2_mixer>
)

include(cmake/Installation.cmake)

add_library(sgg::sgg ALIAS sgg)

add_executable(demo
    demo/demo.cpp
)

target_include_directories(demo
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/demo>
)

target_link_libraries(demo
    PRIVATE
    sgg::sgg
)
