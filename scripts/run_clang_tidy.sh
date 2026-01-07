#!/bin/bash

CLANG_TIDY="/mnt/c/Program Files/LLVM/bin/clang-tidy.exe"
BUILD_DIR="C:\\Users\\g\\avva\\projects\\JUCECmakeRepoPrototype\\out\\build\\x64-Debug"
PROJECT_DIR="C:\\Users\\g\\avva\\projects\\JUCECmakeRepoPrototype"

# Build list of Windows-style paths
FILES=()
for file in $(find Apps/mbk -name "*.cpp" -not -path "*/Libs/*" -not -name "*Test.cpp"); do
    WIN_FILE="${PROJECT_DIR}\\$(echo "$file" | tr '/' '\\')"
    FILES+=("$WIN_FILE")
done

echo "Checking ${#FILES[@]} files:"
printf '%s\n' "${FILES[@]}"
echo ""
"$CLANG_TIDY" -p "$BUILD_DIR" -header-filter="Apps/mbk/.*" "${FILES[@]}" 2>&1
