#!/bin/bash
# Wrapper script to invoke Windows CMake from WSL.
# Usage: ./scripts/cmake.sh [cmake arguments...]
#
# Examples:
#   ./scripts/cmake.sh -G "Visual Studio 17 2022" -B build-windows
#   ./scripts/cmake.sh --build build-windows --config Debug --target mbk

CMAKE_PATH="/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"

if [[ ! -x "$CMAKE_PATH" ]]; then
    echo "Error: Windows CMake not found at: $CMAKE_PATH" >&2
    exit 1
fi

"$CMAKE_PATH" "$@"
