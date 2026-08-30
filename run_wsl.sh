#!/bin/bash
set -e

cmake --build build/windows -j

cp build/windows/spolay.exe incoming/spolay.exe
cp build/windows/third_party/sdl/SDL3.dll incoming/SDL3.dll

LINK_PATH="$(readlink incoming)/spolay.exe"
EXE_PATH="$(wslpath -w $LINK_PATH)"

echo "$EXE_PATH"
# /mnt/c/Windows/System32/cmd.exe /C start "" "$EXE_PATH"
