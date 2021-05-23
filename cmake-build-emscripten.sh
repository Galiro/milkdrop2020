#!/bin/bash 
set -e
cmake   -S . \
        -B build.emscripten \
        -DCMAKE_TOOLCHAIN_FILE=${EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -C build.emscripten
