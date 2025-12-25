#!/bin/bash

CC=g++
LD=g++
AR=ar
BUILD_PATH="build/Release"
BUILD_PATH_DEBUG="build/Debug"
LIB_PATH="lib"
CFLAGS="-O2 -std=c++17"
CFLAGS_DEBUG="-Og -g -std=c++17"

echo "[INFO] Starting SGG build (Release + Debug)"
echo "[INFO] Compiler: $CC | Archiver: $AR"
echo "[INFO] CFLAGS: $CFLAGS"
echo "[INFO] CFLAGS_DEBUG: $CFLAGS_DEBUG"

mkdir -p build
mkdir -p $LIB_PATH
mkdir -p $BUILD_PATH
mkdir -p $BUILD_PATH_DEBUG
mkdir -p $BUILD_PATH/sgg
mkdir -p $BUILD_PATH_DEBUG/sgg

echo "[INFO] Created directories:"
echo "       $(pwd)/build"
echo "       $(pwd)/$LIB_PATH"
echo "       $(pwd)/$BUILD_PATH/sgg"
echo "       $(pwd)/$BUILD_PATH_DEBUG/sgg"

echo_compile_release_prefix="[BUILD] (Release)"
echo_compile_debug_prefix="[BUILD] (Debug)"

echo "$echo_compile_release_prefix Compiling sources to $(pwd)/$BUILD_PATH/sgg"
$CC -c $CFLAGS -I. -I3rdparty/include sgg/graphics.cpp -o $BUILD_PATH/sgg/graphics.o
echo "$echo_compile_release_prefix sgg/graphics.cpp -> $(pwd)/$BUILD_PATH/sgg/graphics.o"
$CC -c $CFLAGS -I. -I3rdparty/include sgg/GLbackend.cpp -o $BUILD_PATH/sgg/GLbackend.o
echo "$echo_compile_release_prefix sgg/GLbackend.cpp -> $(pwd)/$BUILD_PATH/sgg/GLbackend.o"
$CC -c $CFLAGS -I. -I3rdparty/include sgg/shader.cpp -o $BUILD_PATH/sgg/shader.o
echo "$echo_compile_release_prefix sgg/shader.cpp -> $(pwd)/$BUILD_PATH/sgg/shader.o"
$CC -c $CFLAGS -I. -I3rdparty/include sgg/texture.cpp -o $BUILD_PATH/sgg/texture.o
echo "$echo_compile_release_prefix sgg/texture.cpp -> $(pwd)/$BUILD_PATH/sgg/texture.o"
$CC -c $CFLAGS -I. -I3rdparty/include sgg/audio.cpp -o $BUILD_PATH/sgg/audio.o
echo "$echo_compile_release_prefix sgg/audio.cpp -> $(pwd)/$BUILD_PATH/sgg/audio.o"
$CC -c $CFLAGS -I. -I3rdparty/include sgg/AudioManager.cpp -o $BUILD_PATH/sgg/AudioManager.o
echo "$echo_compile_release_prefix sgg/AudioManager.cpp -> $(pwd)/$BUILD_PATH/sgg/AudioManager.o"
$CC -c $CFLAGS -I. -I3rdparty/include sgg/lodepng.cpp -o $BUILD_PATH/sgg/lodepng.o
echo "$echo_compile_release_prefix sgg/lodepng.cpp -> $(pwd)/$BUILD_PATH/sgg/lodepng.o"
$CC -c $CFLAGS -I. -I3rdparty/include sgg/fonts.cpp -o $BUILD_PATH/sgg/fonts.o
echo "$echo_compile_release_prefix sgg/fonts.cpp -> $(pwd)/$BUILD_PATH/sgg/fonts.o"

$AR rcs $LIB_PATH/libsgg.a $BUILD_PATH/sgg/*.o
echo "[OUTPUT✅] Created static library: $(pwd)/$LIB_PATH/libsgg.a"

echo "$echo_compile_debug_prefix Compiling sources to $(pwd)/$BUILD_PATH_DEBUG/sgg"
$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/graphics.cpp -o $BUILD_PATH_DEBUG/sgg/graphics.o
echo "$echo_compile_debug_prefix sgg/graphics.cpp -> $(pwd)/$BUILD_PATH_DEBUG/sgg/graphics.o"
$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/GLbackend.cpp -o $BUILD_PATH_DEBUG/sgg/GLbackend.o
echo "$echo_compile_debug_prefix sgg/GLbackend.cpp -> $(pwd)/$BUILD_PATH_DEBUG/sgg/GLbackend.o"
$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/shader.cpp -o $BUILD_PATH_DEBUG/sgg/shader.o
echo "$echo_compile_debug_prefix sgg/shader.cpp -> $(pwd)/$BUILD_PATH_DEBUG/sgg/shader.o"
$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/texture.cpp -o $BUILD_PATH_DEBUG/sgg/texture.o
echo "$echo_compile_debug_prefix sgg/texture.cpp -> $(pwd)/$BUILD_PATH_DEBUG/sgg/texture.o"
$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/audio.cpp -o $BUILD_PATH_DEBUG/sgg/audio.o
echo "$echo_compile_debug_prefix sgg/audio.cpp -> $(pwd)/$BUILD_PATH_DEBUG/sgg/audio.o"
$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/AudioManager.cpp -o $BUILD_PATH_DEBUG/sgg/AudioManager.o
echo "$echo_compile_debug_prefix sgg/AudioManager.cpp -> $(pwd)/$BUILD_PATH_DEBUG/sgg/AudioManager.o"
$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/lodepng.cpp -o $BUILD_PATH_DEBUG/sgg/lodepng.o
echo "$echo_compile_debug_prefix sgg/lodepng.cpp -> $(pwd)/$BUILD_PATH_DEBUG/sgg/lodepng.o"
$CC -c $CFLAGS_DEBUG -I. -I3rdparty/include sgg/fonts.cpp -o $BUILD_PATH_DEBUG/sgg/fonts.o
echo "$echo_compile_debug_prefix sgg/fonts.cpp -> $(pwd)/$BUILD_PATH_DEBUG/sgg/fonts.o"

$AR rcs $LIB_PATH/libsggd.a $BUILD_PATH_DEBUG/sgg/*.o
echo "[OUTPUT✅] Created debug static library: $(pwd)/$LIB_PATH/libsggd.a"
echo "[DONE] SGG build complete"
