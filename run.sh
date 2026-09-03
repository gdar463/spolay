#!/bin/bash
set -e

if [ ! -d build/linux ]; then
    echo "Run configure.sh first."
fi
if [[ ${1,,} == "refresh_romfs" ]] && [ -f build/linux/third_party/romfs/lib/libromfs_resources.cpp ]; then
    rm -f build/linux/third_party/romfs/lib/libromfs-spolay.a build/linux/third_party/romfs/lib/libromfs_resources.cpp
    shift
fi
cmake --build build/linux -j $@
build/linux/spolay

