#!/bin/bash
set -e

if [ -d ./build ]; then
    if [ -d ./build/linux ]; then
        echo -n "build/linux exits, do you want to delete it? [y/N] "
        read linux_choice
        if [[ ${linux_choice,,} == "y" ]]; then
            rm -rf ./build/linux
            if [ -L ./compile_commands.json ]; then
                rm ./compile_commands.json
            fi
        fi
    fi
    if [ -d ./build/windows ]; then
        echo -n "build/windows exits, do you want to delete it? [y/N] "
        read windows_choice
        if [[ ${windows_choice,,} == "y" ]]; then
            rm -rf ./build/windows
        fi
    fi
else 
    mkdir build
fi
cmake -B build/linux -DCMAKE_BUILD_TYPE=Debug $@
if [ ! -L ./compile_commands.json ]; then
    ln -s "$(pwd)/build/linux/compile_commands.json" compile_commands.json
fi
cmake -B build/windows -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/windows-mingw.cmake -DCMAKE_BUILD_TYPE=Debug $@
