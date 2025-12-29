# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

Configure and build (from project root):
```bash
# Debug build
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug

# Release build
cmake -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release

# Build specific target (faster when working on one app)
cmake --build cmake-build-debug --target mbk
cmake --build cmake-build-debug --target GuiAppTemplate
cmake --build cmake-build-debug --target ConsoleAppTemplate
```

Windows build from WSL (avoids Linux dependency issues):
```bash
# Configure (one time)
"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" -G "Visual Studio 17 2022" -B build-windows

# Build all
"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build-windows --config Debug

# Build single target (preferred when editing one app)
"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build-windows --config Debug --target mbk
```

IDE project generation:
```bash
# Windows Visual Studio
cmake -G "Visual Studio 17 2022" -B build

# macOS Xcode
cmake -G Xcode -B build
```

Use local JUCE instead of fetching from git:
```bash
cmake -DCPM_JUCE_SOURCE="/path/to/JUCE" -B build
```

## Testing

Tests use Catch2 and are disabled by default:
```bash
cmake -B build -DBUILD_UNIT_TESTS=ON
cmake --build build
ctest --test-dir build
```

## Architecture

This is a JUCE 7 multi-project CMake repository using CPM.cmake for package management.

**Root CMakeLists.txt**: Configures JUCE via `find_package(juce)` which delegates to `CMake/Findjuce.cmake` to fetch JUCE using CPM.

**Apps/**: Application targets added via `add_subdirectory()`. Each app has its own CMakeLists.txt using JUCE's CMake API:
- `juce_add_gui_app()` for GUI applications
- `juce_add_console_app()` for console applications

**Modules/**: Custom JUCE-style modules shared across apps, registered with `juce_add_modules()`:
- `custom_module_test`
- `shared_processing_code`
- `shared_plugin_helpers`

**Tests/**: Optional Catch2 unit tests, enabled with `BUILD_UNIT_TESTS=ON`.

**mbk app**: Main audio application with external dependencies (FFTW, libsamplerate) that must be installed separately in `dependencies/` relative to parent directory.

## Code Style

Uses clang-format with Allman brace style, 4-space indentation, 90 column limit. See `_clang-format` for full configuration.

## Linting

**After making code changes, run linting to catch issues:**

```bash
# Lint all source files
./scripts/lint.sh

# Lint specific files
./scripts/lint.sh Apps/mbk/Source/Main.cpp

# Auto-fix issues
./scripts/lint.sh --fix

# Check formatting only
./scripts/lint.sh --format

# Fix formatting
./scripts/lint.sh --format-fix
```

Uses clang-tidy with checks configured in `.clang-tidy`. Requires `compile_commands.json` (auto-generated during cmake configure).

**IDE Integration:**
- VS Code: Install "clangd" extension, it uses `.clang-tidy` automatically
- CLion: Built-in clang-tidy support, enable in Settings > Editor > Inspections
- Visual Studio: Built-in since VS 2019, enable in Project Properties > Code Analysis

## Workflow

After any code changes:
1. Build to verify compilation: `cmake --build cmake-build-debug`
2. Run linting: `./scripts/lint.sh`
3. Fix any issues before committing
