#!/bin/bash
set -e

if [[ ${1,,} == "release" ]]; then
    folder=build/linux/release
    shift
else
    folder=build/linux/debug
fi

if [ ! -d $folder ]; then
    echo "Folder $folder doesn't exist. Run configure.sh first."
fi
if [[ ${1,,} == "refresh_embdfs" ]] && [ -d $folder/third_party/embdfs/generator ]; then
    rm -f $folder/third_party/embdfs/generator $folder/third_party/embdfs/generator-prefix
    shift
fi
if [[ ${1,,} == "only_build" ]]; then
    only_build=1
    shift
else
    only_build=0
fi
cmake --build $folder -j4 $@
if [ $only_build = 0 ]; then
    $folder/spolay
fi

