#!/bin/bash

CLANG_TIDY="/mnt/c/Program Files/JetBrains/CLion 2022.3.3/bin/clang/win/x64/clang-tidy.exe"
BUILD_DIR="C:\\Users\\g\\avva\\projects\\JUCECmakeRepoPrototype\\out\\build\\x64-Debug"
PROJECT_DIR="C:\\Users\\g\\avva\\projects\\JUCECmakeRepoPrototype"

for file in $(find Apps/mbk -name "*.cpp" -not -path "*/Libs/*" -not -name "*Test.cpp"); do
    WIN_FILE="${PROJECT_DIR}\\$(echo "$file" | tr '/' '\\')"
    echo "=== Checking $file ==="
    "$CLANG_TIDY" -p "$BUILD_DIR" "$WIN_FILE" 2>&1 | grep -E "^C:\\\\Users\\\\g\\\\avva\\\\projects\\\\JUCECmakeRepoPrototype\\\\Apps\\\\mbk" | grep -v "clang-diagnostic-error"
done
