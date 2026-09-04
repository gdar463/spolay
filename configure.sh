#!/bin/bash
set -e

if [[ ${1,,} == "only_release" ]]; then
    only_release=1
    shift
else
    only_release=0
fi

if [ -d ./build ]; then
    if [ -d ./build/linux ]; then
        if [ $only_release = 0 ] && [ -d ./build/linux/debug ]; then
            echo -n "build/linux/debug exits, do you want to delete it? [y/N] "
            read linux_debug_choice
            if [[ ${linux_debug_choice,,} == "y" ]]; then
                rm -rf ./build/linux/debug
                if [ -L ./compile_commands.json ]; then
                    rm ./compile_commands.json
                fi
            fi
        fi
        if [ -d ./build/linux/release ]; then
            echo -n "build/linux/release exits, do you want to delete it? [y/N] "
            read linux_release_choice
            if [[ ${linux_release_choice,,} == "y" ]]; then
                rm -rf ./build/linux/release
            fi
        fi
    else
        mkdir ./build/linux
    fi
    if [ -d ./build/windows ]; then
        if [ $only_release = 0 ] && [ -d ./build/windows/debug ]; then
            echo -n "build/windows/debug exits, do you want to delete it? [y/N] "
            read windows_debug_choice
            if [[ ${windows_debug_choice,,} == "y" ]]; then
                rm -rf ./build/windows/debug
            fi
        fi
        if [ -d ./build/windows/release ]; then
            echo -n "build/windows/release exits, do you want to delete it? [y/N] "
            read windows_release_choice
            if [[ ${windows_release_choice,,} == "y" ]]; then
                rm -rf ./build/windows/release
            fi
        fi
    else
        mkdir ./build/windows
    fi
else 
    mkdir build
    mkdir build/linux
    mkdir build/windows
fi

if [ $only_release = 0 ]; then
    cmake -B build/linux/debug -DCMAKE_BUILD_TYPE=Debug $@
    if [ ! -L ./compile_commands.json ]; then
        ln -s "$(pwd)/build/linux/debug/compile_commands.json" compile_commands.json
    fi
    LINK_PATH="$(readlink incoming)"
    REAL_PATH="$(wslpath -w $LINK_PATH)"
    cmake -B build/windows/debug -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/windows-mingw.cmake -DCMAKE_BUILD_TYPE=Debug -DINCOMING_DIR=$REAL_PATH $@
fi
cmake -B build/linux/release -DCMAKE_BUILD_TYPE=Release $@
cmake -B build/windows/release -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/windows-mingw.cmake -DCMAKE_BUILD_TYPE=Release $@
