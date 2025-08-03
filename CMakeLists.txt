# Project configuration
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(sgg VERSION 2.0)

# Option to build test executable (OFF by default)
option(BUILD_TESTS "Build test executable" OFF)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define include directories for third-party headers
set(THIRD_PARTY_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/3rdparty/include)

# Define library path for third-party .lib files
set(THIRD_PARTY_LIB_PATH ${CMAKE_SOURCE_DIR}/3rdparty/lib)

# Automatically gather all .cpp and .h files recursively under sgg/
file(GLOB_RECURSE SGG_SOURCES
        ${CMAKE_SOURCE_DIR}/sgg/*.cpp
        ${CMAKE_SOURCE_DIR}/sgg/*.h
)

#------------------ Platform / Toolchain Configuration ------------------

if (WIN32 AND MINGW)
    message(STATUS "Configuring for Windows MinGW build")

    # Library directories for MinGW toolchain on Windows
    set(MINGW64_LIB_DIR "C:/msys64/mingw64/lib")
    set(UCRT64_LIB_DIR "C:/msys64/ucrt64/lib")
    link_directories(${MINGW64_LIB_DIR} ${UCRT64_LIB_DIR})

    # Define common MinGW libraries (static linkage preferred)
    set(LIBS
        -lSDL2_mixer -lSDL2main -lSDL2_test
        -lSDL2 -lglew32 -lfreetype -lharfbuzz
        -lgraphite2 -lbrotlidec -lbrotlicommon
        -lmpg123 -lopusfile -lopus -logg -lopengl32
        -lwinmm -limm32 -lsetupapi -lcfgmgr32 -lversion
        -lrpcrt4 -lshlwapi -lmingwex -lpng16 -lz -lbz2)

    # Compiler/linker flags
    set(COMPILER_WARNINGS -Wall -Wextra -Wunused-function -g -O2)
    set(LINKER_FLAGS "")

elseif (WIN32) # MSVC (cl)
    message(STATUS "Configuring for Windows MSVC build")

    # Copy assets / binaries only when building tests (same as before)
    if (BUILD_TESTS)
        file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR}/bin)
        file(GLOB BIN_FILES ${CMAKE_SOURCE_DIR}/3rdparty/bin/*.dll ${CMAKE_SOURCE_DIR}/3rdparty/bin/*.exe)
        foreach(BIN_FILE ${BIN_FILES})
            file(COPY ${BIN_FILE} DESTINATION ${CMAKE_BINARY_DIR}/bin)
        endforeach()
        if (EXISTS ${CMAKE_SOURCE_DIR}/3rdparty/bin/MSVC)
            file(COPY ${CMAKE_SOURCE_DIR}/3rdparty/bin/MSVC/ DESTINATION ${CMAKE_BINARY_DIR}/bin FILES_MATCHING PATTERN "*.dll" PATTERN "*.exe")
        endif()
    endif()

    # MSVC library set (.lib)
    set(LIBS
        ${THIRD_PARTY_LIB_PATH}/MSVC/SDL2_mixer.lib
        ${THIRD_PARTY_LIB_PATH}/MSVC/SDL2main.lib
        ${THIRD_PARTY_LIB_PATH}/MSVC/SDL2test.lib
        ${THIRD_PARTY_LIB_PATH}/MSVC/glew32s.lib
        ${THIRD_PARTY_LIB_PATH}/MSVC/SDL2.lib
        ${THIRD_PARTY_LIB_PATH}/MSVC/freetype.lib
        ${THIRD_PARTY_LIB_PATH}/MSVC/OpenGL32.Lib)

    set(COMPILER_WARNINGS /Wall)
    set(LINKER_FLAGS "")

elseif (UNIX)
    message(STATUS "Configuring for Linux GCC/MinGW-compatible build")

    # On Linux we assume native GCC toolchain with .so/.a libraries provided by the system
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
    pkg_check_modules(GLEW REQUIRED glew)
    pkg_check_modules(FREETYPE REQUIRED freetype2)
    find_package(OpenGL REQUIRED)

    set(LIBS
        ${SDL2_LIBRARIES}
        ${SDL2_MIXER_LIBRARIES}
        ${GLEW_LIBRARIES}
        ${FREETYPE_LIBRARIES}
        ${OPENGL_gl_LIBRARY})

    # Include dirs for pkg-config packages
    include_directories(
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_MIXER_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
        ${FREETYPE_INCLUDE_DIRS})

    set(COMPILER_WARNINGS -Wall -Wextra -Wpedantic)
    set(LINKER_FLAGS "")

else()
    message(FATAL_ERROR "Unsupported platform/toolchain configuration")
endif()

#------------------ Target Creation ------------------
if(BUILD_TESTS)
    message(STATUS "Building test executable including test.cpp")
    set(TEST_SOURCE ${CMAKE_SOURCE_DIR}/sgg/test/test.cpp)
    add_executable(${PROJECT_NAME} ${TEST_SOURCE} ${SGG_SOURCES})

else()
    message(STATUS "Building library only (test.cpp excluded)")
    add_library(${PROJECT_NAME} STATIC ${SGG_SOURCES})
endif()

# Add compile options after target exists
target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILER_WARNINGS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
        ${THIRD_PARTY_INCLUDE_PATH}
        ${THIRD_PARTY_INCLUDE_PATH}/SDL2
        ${CMAKE_SOURCE_DIR}/3rdparty
        ${CMAKE_SOURCE_DIR}/sgg
)

# Link external libraries
target_link_libraries(${PROJECT_NAME} ${LIBS})

# Linker flags (only for MinGW with coverage)
if(LINKER_FLAGS)
    target_link_options(${PROJECT_NAME} PRIVATE ${LINKER_FLAGS})
endif()

# Compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE GLEW_STATIC)

# If executable, set runtime output dir
if(BUILD_TESTS)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()