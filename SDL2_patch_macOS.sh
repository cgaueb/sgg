#!/bin/bash
# @gsiros patch
echo "Modifying SDL2_stdinc.h to fit macOS..."
cp 3rdparty/include/SDL2/SDL_stdinc.h 3rdparty/include/SDL2/SDL_stdinc.h.ORIGINAL
mv 3rdparty/include/SDL2/SDL_stdinc.h.ORIGINAL 3rdparty/MACOS_SUPPORT
mv 3rdparty/MACOS_SUPPORT/SDL_stdinc.h 3rdparty/include/SDL2
echo "DONE!"