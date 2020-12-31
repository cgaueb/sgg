#!/bin/bash

# @gsiros modifications

CC=g++
LD=g++
AR=ar
BUILD_PATH="build/Release"
BUILD_PATH_DEBUG="build/Debug"
LIB_PATH="lib"
CFLAGS="-O2"
CFLAGS_DEBUG="-Og -g"

mkdir -p build
mkdir -p $LIB_PATH
mkdir -p $BUILD_PATH
mkdir -p $BUILD_PATH_DEBUG
mkdir -p $BUILD_PATH/sgg
mkdir -p $BUILD_PATH_DEBUG/sgg

echo "-- START OF SGG LIB (RELEASE) COMPILATION --"
$CC -c -std=c++11 $CFLAGS -I. -I3rdparty/include sgg/graphics.cpp -o $BUILD_PATH/sgg/graphics.o
echo "Compiled graphics!"
$CC -c -std=c++11 $CFLAGS -I. -I3rdparty/include sgg/GLbackend.cpp -o $BUILD_PATH/sgg/GLbackend.o
echo "Compiled GLbackend!"
$CC -c -std=c++11 $CFLAGS -I. -I3rdparty/include sgg/shader.cpp -o $BUILD_PATH/sgg/shader.o
echo "Compiled shader!"
$CC -c -std=c++11 $CFLAGS -I. -I3rdparty/include sgg/texture.cpp -o $BUILD_PATH/sgg/texture.o
echo "Compiled texture!"
$CC -c -std=c++11 $CFLAGS -I. -I3rdparty/include sgg/audio.cpp -o $BUILD_PATH/sgg/audio.o
echo "Compiled audio!"
$CC -c -std=c++11 $CFLAGS -I. -I3rdparty/include sgg/AudioManager.cpp -o $BUILD_PATH/sgg/AudioManager.o
echo "Compiled AudioManager!"
$CC -c -std=c++11 $CFLAGS -I. -I3rdparty/include sgg/lodepng.cpp -o $BUILD_PATH/sgg/lodepng.o
echo "Compiled lodepng!"
$CC -c -std=c++11 $CFLAGS -I. -I3rdparty/include sgg/fonts.cpp -o $BUILD_PATH/sgg/fonts.o
echo "Compiled fonts!"

$AR rcs $LIB_PATH/libsgg.a $BUILD_PATH/sgg/*.o

echo "-- START OF SGG LIB (DEBUG) COMPILATION --"
$CC -c -std=c++11 $CFLAGS_DEBUG -I. -I3rdparty/include sgg/graphics.cpp -o $BUILD_PATH_DEBUG/sgg/graphics.o
echo "Compiled graphics!"
$CC -c -std=c++11 $CFLAGS_DEBUG -I. -I3rdparty/include sgg/GLbackend.cpp -o $BUILD_PATH_DEBUG/sgg/GLbackend.o
echo "Compiled GLbackend!"
$CC -c -std=c++11 $CFLAGS_DEBUG -I. -I3rdparty/include sgg/shader.cpp -o $BUILD_PATH_DEBUG/sgg/shader.o
echo "Compiled shader!"
$CC -c -std=c++11 $CFLAGS_DEBUG -I. -I3rdparty/include sgg/texture.cpp -o $BUILD_PATH_DEBUG/sgg/texture.o
echo "Compiled texture!"
$CC -c -std=c++11 $CFLAGS_DEBUG -I. -I3rdparty/include sgg/audio.cpp -o $BUILD_PATH_DEBUG/sgg/audio.o
echo "Compiled audio!"
$CC -c -std=c++11 $CFLAGS_DEBUG -I. -I3rdparty/include sgg/AudioManager.cpp -o $BUILD_PATH_DEBUG/sgg/AudioManager.o
echo "Compiled AudioManager!"
$CC -c -std=c++11 $CFLAGS_DEBUG -I. -I3rdparty/include sgg/lodepng.cpp -o $BUILD_PATH_DEBUG/sgg/lodepng.o
echo "Compiled lodepng!"
$CC -c -std=c++11 $CFLAGS_DEBUG -I. -I3rdparty/include sgg/fonts.cpp -o $BUILD_PATH_DEBUG/sgg/fonts.o
echo "Compiled fonts!"

$AR rcs $LIB_PATH/libsggd.a $BUILD_PATH_DEBUG/sgg/*.o
