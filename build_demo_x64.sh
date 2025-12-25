#!/bin/bash

CC=g++
LD=g++
BUILD_PATH="build/Release"
BUILD_PATH_DEBUG="build/Debug"
BIN_PATH="bin"
LIB_PATH="lib"
CFLAGS="-O2 -std=c++17"
CFLAGS_DEBUG="-Og -g -std=c++17"

echo "[INFO] Building demo binaries (Release + Debug)"
echo "[INFO] Compiler: $CC"
echo "[INFO] Using libraries from: $(pwd)/$LIB_PATH"
echo "[INFO] CFLAGS: $CFLAGS"
echo "[INFO] CFLAGS_DEBUG: $CFLAGS_DEBUG"

mkdir -p build
mkdir -p $BIN_PATH
mkdir -p $BIN_PATH/assets
mkdir -p $BUILD_PATH
mkdir -p $BUILD_PATH_DEBUG

echo "[INFO] Created directories:"
echo "       $(pwd)/build"
echo "       $(pwd)/$BUILD_PATH"
echo "       $(pwd)/$BUILD_PATH_DEBUG"
echo "       $(pwd)/$BIN_PATH"
echo "       $(pwd)/$BIN_PATH/assets"

$CC $CFLAGS -L$LIB_PATH -I. demo/demo.cpp -o $BIN_PATH/demo -lsgg -lGL -lGLEW -lSDL2 -lSDL2_mixer -lfreetype -lstdc++fs
echo "[OUTPUT] Built Release demo: $(pwd)/$BIN_PATH/demo"

$CC $CFLAGS_DEBUG -L$LIB_PATH -I. demo/demo.cpp -o $BIN_PATH/demod -lsggd -lGL -lGLEW -lSDL2 -lSDL2_mixer -lfreetype -lstdc++fs
echo "[OUTPUT] Built Debug demo:   $(pwd)/$BIN_PATH/demod"

cp -r 3rdparty/assets $BIN_PATH
echo "[COPY] Assets copied to: $(pwd)/$BIN_PATH/assets"
echo "[DONE] Demo build complete"
