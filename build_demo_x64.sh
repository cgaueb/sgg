#!/bin/bash

CC=g++
LD=g++
BUILD_PATH="build/Release"
BUILD_PATH_DEBUG="build/Debug"
BIN_PATH="bin"
LIB_PATH="lib"
CFLAGS="-O2 -std=c++17"
CFLAGS_DEBUG="-Og -g -std=c++17"

mkdir -p build
mkdir -p $BIN_PATH
mkdir -p $BIN_PATH/assets
mkdir -p $BUILD_PATH
mkdir -p $BUILD_PATH_DEBUG

$CC $CFLAGS -L$LIB_PATH -I. demo/demo.cpp -o $BIN_PATH/demo -lsgg -lGL -lGLEW -lSDL2 -lSDL2_mixer -lfreetype -lstdc++fs

$CC $CFLAGS_DEBUG -L$LIB_PATH -I. demo/demo.cpp -o $BIN_PATH/demod -lsggd -lGL -lGLEW -lSDL2 -lSDL2_mixer -lfreetype -lstdc++fs

cp -r 3rdparty/assets $BIN_PATH
