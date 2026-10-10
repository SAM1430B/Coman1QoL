#!/bin/sh
# Build winmm.dll and the *.c1m modules with MinGW-w64 (32-bit) on Linux / WSL / MSYS2:
#   apt install g++-mingw-w64-i686   (or: pacman -S mingw-w64-i686-gcc)
# On Windows with Visual Studio, open Coman1QoL.sln instead.
set -e
cd "$(dirname "$0")"
CXX=${CXX:-i686-w64-mingw32-g++}
FLAGS="-std=c++20 -O2 -Wall -Wno-unused-parameter -fno-exceptions -fno-rtti -shared -s -static -ICommon"
mkdir -p build
$CXX $FLAGS -o build/winmm.dll Common/*.cpp WinmmProxy/*.cpp WinmmProxy/winmm.def -Wl,--enable-stdcall-fixup
$CXX $FLAGS -o build/Resolution.c1m Common/*.cpp ComanResolution/*.cpp
echo "built build/winmm.dll and build/Resolution.c1m"
