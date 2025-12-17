#!/bin/bash

set -e 

DO_BUILD=false
DO_CMAKE=false
DO_TESTS=false

for arg in "$@"; do
  case $arg in
    -b|--build)
      DO_BUILD=true
      ;;
    -c|--cmake)
      DO_CMAKE=true
      ;;
    -t|--test)
      DO_TESTS=true
      ;;
  esac
done


if [ ! -d "build" ] || [ "$DO_CMAKE" = true ]; then
  echo "[+] Configuring with CMake..."
  cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi

if [ "$DO_BUILD" = true ]; then
  echo "[+] Building..."
  ninja -C build
fi

if [ "$DO_TESTS" = true ]; then
  echo "[+] Testing..."
  ./build/tests/lexer_unit_tests
else
  echo "[+] Running..."
  ./build/AssemblyCompiler
fi
