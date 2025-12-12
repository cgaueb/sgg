#!/bin/bash

# @dmarakom6 modifications of @gsiros's shell script build_demo_macOS.sh for Silicon Macs

# Install dependencies if missing
echo "Checking dependencies..."
brew install glew sdl2 sdl2_mixer freetype


CC=g++
LD=g++
BUILD_PATH="build/Release"
BUILD_PATH_DEBUG="build/Debug"
BIN_PATH="bin"
LIB_PATH="lib"
CFLAGS="-O2 -arch x86_64"
CFLAGS_DEBUG="-Og -g -arch x86_64"

mkdir -p build
mkdir -p $BIN_PATH
mkdir -p $BIN_PATH/assets
mkdir -p $BUILD_PATH
mkdir -p $BUILD_PATH_DEBUG

compile_demo() {
    local target_name=$1
    local cflags=$2
    local sgg_lib=$3
    local build_path=$4
    echo "Compiling $target_name..."
    $CC -std=c++17 $cflags -L$LIB_PATH -L$GLEW_LIB_PATH -I. demo/demo.cpp -o $BIN_PATH/$target_name -l$sgg_lib -lGLEW -lSDL2 -lSDL2_mixer -lfreetype -framework OpenGL
}

GLEW_LIB_PATH="$(brew --prefix)/opt/glew/lib"

compile_demo demo "$CFLAGS" sgg "$BUILD_PATH"
compile_demo demod "$CFLAGS_DEBUG" sggd "$BUILD_PATH_DEBUG"

cp -r 3rdparty/assets $BIN_PATH


