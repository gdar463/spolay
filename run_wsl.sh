#!/bin/bash
set -e

if [[ ${1,,} == "--help" ]] || [[ ${1,,} == "-h" ]] || [[ ${1,,} == "-?" ]]; then
    echo "usage: run_wsl.sh [--help] [release] [refresh_embdfs]\n"
    echo "USE ARGS ONLY IN THE ORDER SHOWN"
    exit 0
fi

if [[ ${1,,} == "release" ]]; then
    folder=build/windows/release
    shift
else
    folder=build/windows/debug
    cp -rf src incoming/src
fi

if [ ! -d $folder ]; then
    echo "Folder $folder doesn't exist. Run configure.sh first."
fi
if [[ ${1,,} == "refresh_embdfs" ]] && [ -d $folder/third_party/embdfs/generator ]; then
    rm -f $folder/third_party/embdfs/generator $folder/third_party/embdfs/generator-prefix
    shift
fi

cmake --build $folder -j4

cp $folder/spolay.exe incoming/spolay.exe
cp $folder/third_party/sdl/SDL3.dll incoming/SDL3.dll

LINK_PATH="$(readlink incoming)/spolay.exe"
EXE_PATH="$(wslpath -w $LINK_PATH)"

echo "$EXE_PATH"
# /mnt/c/Windows/System32/cmd.exe /C start "" "$EXE_PATH"
