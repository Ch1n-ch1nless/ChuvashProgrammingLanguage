#!/usr/bin/env bash

set -e

mkdir -p build

cd build

cmake .. -DLLVM_DIR=../build-llvm/lib/cmake/llvm

ninja