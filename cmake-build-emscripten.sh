#!/bin/bash 
set -e

export CMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} -sUSE_SDL=2"

cmake   -S . \
        -B build.emscripten \
        -DCMAKE_TOOLCHAIN_FILE=${EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -C build.emscripten


