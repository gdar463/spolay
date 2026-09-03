#!/bin/bash
set -e

if [ ! -d build/linux ]; then
    echo "Run configure.sh first."
fi
if [[ ${1,,} == "refresh_romfs" ]] && [ -f build/linux/third_party/romfs/lib/libromfs_resources.cpp ]; then
    rm -f build/linux/third_party/romfs/lib/libromfs-spolay.a build/linux/third_party/romfs/lib/libromfs_resources.cpp
    shift
fi
if [[ ${1,,} == "only_build" ]]; then
    only_build=1
    shift
else
    only_build=0
fi
cmake --build build/linux -j $@
if [ $only_build = 0 ]; then
    build/linux/spolay
fi

