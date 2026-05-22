# ChuvashProgrammingLanguage
My programming language and compiler for it.

## Requirements:

- CMake
- Ninja
- Clang

## Clone:

```bash
git clone --recursive <repo>
```

If you forgot --recursive:

```bash
git submodule update --init --recursive
```

## Build:

```bash
# Firstly, you need to build LLVM:
./build-llvm.sh

# Build all project:
./build.sh
```