#!/bin/bash

CC=g++
LD=g++
AR=ar
BUILD_PATH="build/Release"
BUILD_PATH_DEBUG="build/Debug"
LIB_PATH="lib"
CFLAGS="-O2 -std=c++17"
CFLAGS_DEBUG="-Og -g -std=c++17"

mkdir -p build
mkdir -p $LIB_PATH
mkdir -p $BUILD_PATH
mkdir -p $BUILD_PATH_DEBUG
mkdir -p $BUILD_PATH/sgg
mkdir -p $BUILD_PATH_DEBUG/sgg

$CC -c $CFLAGS -I. -I3rdparty/include sgg/graphics.cpp -o $BUILD_PATH/sgg/graphics.o
$CC -c $CFLAGS -I. -I3rdparty/include sgg/GLbackend.cpp -o $BUILD_PATH/sgg/GLbackend.o
$CC -c $CFLAGS -I. -I3rdparty/include sgg/shader.cpp -o $BUILD_PATH/sgg/shader.o
$CC -c $CFLAGS -I. -I3rdparty/include sgg/texture.cpp -o $BUILD_PATH/sgg/texture.o
$CC -c $CFLAGS -I. -I3rdparty/include sgg/audio.cpp -o $BUILD_PATH/sgg/audio.o
$CC -c $CFLAGS -I. -I3rdparty/include sgg/AudioManager.cpp -o $BUILD_PATH/sgg/AudioManager.o
$CC -c $CFLAGS -I. -I3rdparty/include sgg/lodepng.cpp -o $BUILD_PATH/sgg/lodepng.o
$CC -c $CFLAGS -I. -I3rdparty/include sgg/fonts.cpp -o $BUILD_PATH/sgg/fonts.o

$AR rcs $LIB_PATH/libsgg.a $BUILD_PATH/sgg/*.o

$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/graphics.cpp -o $BUILD_PATH_DEBUG/sgg/graphics.o
$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/GLbackend.cpp -o $BUILD_PATH_DEBUG/sgg/GLbackend.o
$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/shader.cpp -o $BUILD_PATH_DEBUG/sgg/shader.o
$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/texture.cpp -o $BUILD_PATH_DEBUG/sgg/texture.o
$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/audio.cpp -o $BUILD_PATH_DEBUG/sgg/audio.o
$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/AudioManager.cpp -o $BUILD_PATH_DEBUG/sgg/AudioManager.o
$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/lodepng.cpp -o $BUILD_PATH_DEBUG/sgg/lodepng.o
$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/fonts.cpp -o $BUILD_PATH_DEBUG/sgg/fonts.o

$AR rcs $LIB_PATH/libsggd.a $BUILD_PATH_DEBUG/sgg/*.o
