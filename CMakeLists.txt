# Project configuration
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(sgg VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define include directories for third-party headers
set(THIRD_PARTY_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/3rdparty/include)

# Define library path for third-party .lib files
set(THIRD_PARTY_LIB_PATH ${CMAKE_SOURCE_DIR}/3rdparty/lib)

# Automatically gather all .cpp and .h files in sgg and sgg/headers directories
file(GLOB_RECURSE SGG_SOURCES
        ${CMAKE_SOURCE_DIR}/sgg/*.cpp
        ${CMAKE_SOURCE_DIR}/sgg/headers/*.h
)

# Copy all files from 3rdparty/bin to the build output directory
file(COPY ${CMAKE_SOURCE_DIR}/3rdparty/bin DESTINATION ${CMAKE_BINARY_DIR})


# Define the list of third-party libraries to link (.lib files from 3rdparty/lib)
set(LIBS
        ${THIRD_PARTY_LIB_PATH}/SDL2_mixer.lib
        ${THIRD_PARTY_LIB_PATH}/SDL2main.lib
        ${THIRD_PARTY_LIB_PATH}/SDL2test.lib
        ${THIRD_PARTY_LIB_PATH}/glew32.lib
        ${THIRD_PARTY_LIB_PATH}/glew32s.lib
        ${THIRD_PARTY_LIB_PATH}/SDL2.lib
        ${THIRD_PARTY_LIB_PATH}/freetype.lib
        ${THIRD_PARTY_LIB_PATH}/OpenGL32.lib
)

# Add the executable with TEST.CPP along with other source files
#add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/sgg/TEST.CPP ${SGG_SOURCES})
add_library(${PROJECT_NAME} ${CMAKE_SOURCE_DIR} ${SGG_SOURCES})

# Include the third-party include directory
target_include_directories(${PROJECT_NAME} PRIVATE ${THIRD_PARTY_INCLUDE_PATH})

# Link libraries to the executable
target_link_libraries(${PROJECT_NAME} ${LIBS})

# Define output directory for the executable
set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)