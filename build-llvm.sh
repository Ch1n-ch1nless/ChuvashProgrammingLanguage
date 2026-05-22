#!/usr/bin/env bash

set -e

mkdir -p build-llvm

cd build-llvm

cmake -G Ninja ../external/llvm-project/llvm \
    -DLLVM_ENABLE_PROJECTS="clang" \
    -DLLVM_TARGETS_TO_BUILD="X86" \
    -DLLVM_BUILD_TESTS=OFF \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_BUILD_EXAMPLES=OFF \
    -DCMAKE_BUILD_TYPE=Release

ninja