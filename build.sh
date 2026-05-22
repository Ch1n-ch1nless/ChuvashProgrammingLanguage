#!/usr/bin/env bash

set -e

mkdir -p build

cd build

cmake .. -G Ninja -DLLVM_DIR=../build-llvm/lib/cmake/llvm

ninja