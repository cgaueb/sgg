#!/bin/bash

# @gsiros modifications of @ViNeek's make file.

CC=g++
LD=g++
BUILD_PATH="build/Release"
BUILD_PATH_DEBUG="build/Debug"
BIN_PATH="bin"
LIB_PATH="lib"
CFLAGS="-O2"
CFLAGS_DEBUG="-Og -g"

mkdir -p build
mkdir -p $BIN_PATH
mkdir -p $BIN_PATH/assets
mkdir -p $BUILD_PATH
mkdir -p $BUILD_PATH_DEBUG

$CC -std=c++11 $CFLAGS -L$LIB_PATH -I. demo/demo.cpp -o $BIN_PATH/demo -lsgg -lGLEW -lSDL2 -lSDL2_mixer -lfreetype -framework OpenGL

$CC -std=c++11 $CFLAGS_DEBUG -L$LIB_PATH -I. demo/demo.cpp -o $BIN_PATH/demod -lsggd -lGLEW -lSDL2 -lSDL2_mixer -lfreetype -framework OpenGL

cp -r 3rdparty/assets $BIN_PATH
