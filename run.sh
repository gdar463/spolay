#!/bin/bash
set -e

cmake --build build/linux -j $@
build/linux/spolay

