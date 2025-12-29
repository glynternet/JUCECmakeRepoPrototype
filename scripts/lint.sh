#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Default build directory
BUILD_DIR="${BUILD_DIR:-cmake-build-debug}"
COMPILE_COMMANDS="$PROJECT_ROOT/$BUILD_DIR/compile_commands.json"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

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

# Check for compile_commands.json
if [[ ! -f "$COMPILE_COMMANDS" ]]; then
    echo -e "${YELLOW}Warning: compile_commands.json not found at $COMPILE_COMMANDS${NC}"
    echo "Run cmake to generate it:"
    echo "  cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Debug"
    exit 1
fi

# Find source files if none specified
if [[ ${#FILES[@]} -eq 0 ]]; then
    mapfile -t FILES < <(find Apps Modules -type f \( -name "*.cpp" -o -name "*.h" \) 2>/dev/null)
fi

if [[ ${#FILES[@]} -eq 0 ]]; then
    echo -e "${YELLOW}No source files found${NC}"
    exit 0
fi

# Run clang-format
if [[ -n "$FORMAT_ONLY" || -n "$FORMAT_FIX" ]]; then
    echo -e "${GREEN}Running clang-format...${NC}"
    if [[ -n "$FORMAT_FIX" ]]; then
        clang-format -i "${FILES[@]}"
        echo -e "${GREEN}Format fixes applied${NC}"
    else
        if ! clang-format --dry-run --Werror "${FILES[@]}" 2>&1; then
            echo -e "${RED}Format check failed${NC}"
            exit 1
        fi
        echo -e "${GREEN}Format check passed${NC}"
    fi
    if [[ -n "$FORMAT_ONLY" ]]; then
        exit 0
    fi
fi

# Run clang-tidy
echo -e "${GREEN}Running clang-tidy on ${#FILES[@]} files...${NC}"
echo ""

FAILED=0
for file in "${FILES[@]}"; do
    if [[ "$file" == *.cpp ]]; then
        echo -n "Checking $file... "
        if clang-tidy -p "$COMPILE_COMMANDS" $FIX "$file" 2>&1 | grep -v "^$"; then
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
