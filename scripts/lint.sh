#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Default build directory - try several common locations
find_compile_commands() {
    local dirs=("cmake-build-debug" "cmake-build-release" "out/build/x64-Debug" "build-ninja")
    for dir in "${dirs[@]}"; do
        if [[ -f "$PROJECT_ROOT/$dir/compile_commands.json" ]]; then
            echo "$dir"
            return
        fi
    done
    echo "cmake-build-debug"  # fallback to default for error message
}

BUILD_DIR="${BUILD_DIR:-$(find_compile_commands)}"
COMPILE_COMMANDS="$PROJECT_ROOT/$BUILD_DIR/compile_commands.json"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Find clang-format (prefer Linux, fallback to Windows)
find_clang_format() {
    if command -v clang-format &> /dev/null; then
        echo "clang-format"
    elif [[ -f "/mnt/c/Program Files/LLVM/bin/clang-format.exe" ]]; then
        echo "/mnt/c/Program Files/LLVM/bin/clang-format.exe"
    elif [[ -f "/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/bin/clang-format.exe" ]]; then
        echo "/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/bin/clang-format.exe"
    else
        echo ""
    fi
}

# Find clang-tidy (prefer Linux, fallback to Windows)
find_clang_tidy() {
    if command -v clang-tidy &> /dev/null; then
        echo "clang-tidy"
    elif [[ -f "/mnt/c/Program Files/LLVM/bin/clang-tidy.exe" ]]; then
        echo "/mnt/c/Program Files/LLVM/bin/clang-tidy.exe"
    else
        echo ""
    fi
}

CLANG_FORMAT=$(find_clang_format)
CLANG_TIDY=$(find_clang_tidy)

usage() {
    echo "Usage: $0 [OPTIONS] [FILES...]"
    echo ""
    echo "Options:"
    echo "  -b, --build-dir DIR    Build directory containing compile_commands.json"
    echo "                         (default: cmake-build-debug)"
    echo "  -f, --fix              Apply fixes automatically"
    echo "  --format               Run clang-format check"
    echo "  --format-fix           Run clang-format and apply fixes"
    echo "  -h, --help             Show this help message"
    echo ""
    echo "If no FILES are specified, lints all source files in Apps/ and Modules/"
}

FIX=""
FORMAT_ONLY=""
FORMAT_FIX=""
FILES=()

while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--build-dir)
            BUILD_DIR="$2"
            COMPILE_COMMANDS="$PROJECT_ROOT/$BUILD_DIR/compile_commands.json"
            shift 2
            ;;
        -f|--fix)
            FIX="--fix"
            shift
            ;;
        --format)
            FORMAT_ONLY="1"
            shift
            ;;
        --format-fix)
            FORMAT_FIX="1"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            FILES+=("$1")
            shift
            ;;
    esac
done

cd "$PROJECT_ROOT"

# Find source files if none specified
if [[ ${#FILES[@]} -eq 0 ]]; then
    mapfile -t FILES < <(find Apps Modules -type f \( -name "*.cpp" -o -name "*.h" \) -not -path "*/Libs/*" -not -name "*Test.cpp" 2>/dev/null)
fi

if [[ ${#FILES[@]} -eq 0 ]]; then
    echo -e "${YELLOW}No source files found${NC}"
    exit 0
fi

# Run clang-format
if [[ -n "$FORMAT_ONLY" || -n "$FORMAT_FIX" ]]; then
    if [[ -z "$CLANG_FORMAT" ]]; then
        echo -e "${RED}Error: clang-format not found${NC}"
        exit 1
    fi
    echo -e "${GREEN}Running clang-format (${#FILES[@]} files)...${NC}"
    if [[ -n "$FORMAT_FIX" ]]; then
        for file in "${FILES[@]}"; do
            "$CLANG_FORMAT" -i "$file"
        done
        echo -e "${GREEN}Format fixes applied${NC}"
    else
        FAILED=0
        for file in "${FILES[@]}"; do
            if ! "$CLANG_FORMAT" --dry-run --Werror "$file" 2>&1; then
                FAILED=1
            fi
        done
        if [[ $FAILED -eq 1 ]]; then
            echo -e "${RED}Format check failed${NC}"
            exit 1
        fi
        echo -e "${GREEN}Format check passed${NC}"
    fi
    exit 0
fi

# Run clang-tidy
if [[ -z "$CLANG_TIDY" ]]; then
    echo -e "${RED}Error: clang-tidy not found${NC}"
    exit 1
fi

# Check for compile_commands.json
if [[ ! -f "$COMPILE_COMMANDS" ]]; then
    echo -e "${YELLOW}Warning: compile_commands.json not found at $COMPILE_COMMANDS${NC}"
    echo "Run cmake to generate it:"
    echo "  cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Debug"
    exit 1
fi

echo -e "${GREEN}Running clang-tidy on ${#FILES[@]} files...${NC}"
echo ""

FAILED=0
for file in "${FILES[@]}"; do
    if [[ "$file" == *.cpp ]]; then
        echo -n "Checking $file... "
        if "$CLANG_TIDY" -p "$COMPILE_COMMANDS" $FIX "$file" 2>&1 | grep -v "^$"; then
            FAILED=1
        else
            echo -e "${GREEN}OK${NC}"
        fi
    fi
done

if [[ $FAILED -eq 1 ]]; then
    echo ""
    echo -e "${RED}Linting found issues${NC}"
    exit 1
else
    echo ""
    echo -e "${GREEN}All checks passed${NC}"
fi
