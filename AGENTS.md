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
./scripts/cmake.sh -G "Visual Studio 17 2022" -B build-windows

# Build all
./scripts/cmake.sh --build build-windows --config Debug

# Build single target (preferred when editing one app)
./scripts/cmake.sh --build build-windows --config Debug --target mbk
```

Note: `scripts/cmake.sh` is a wrapper that invokes Windows CMake from WSL.

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

Tests use Catch2 and are disabled by default.

**From WSL (recommended):**
```bash
# Configure with tests enabled (one time)
./scripts/cmake.sh -G "Visual Studio 17 2022" -B build-windows -DBUILD_UNIT_TESTS=ON

# Build test runner
./scripts/cmake.sh --build build-windows --config Debug --target UnitTestRunner

# Run tests
./build-windows/Tests/UnitTestRunner_artefacts/Debug/UnitTestRunner.exe
```

**Native (requires all dependencies installed):**
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

Uses clang-format with attached brace style (K&R), 4-space indentation, 90 column limit. See `_clang-format` for full configuration.

**Before writing code, review `.clang-tidy`** to understand enforced checks and naming conventions.

## Naming Conventions

From `.clang-tidy` `readability-identifier-naming` checks:

| Identifier Type | Case Style | Example |
|-----------------|------------|---------|
| Classes | `CamelCase` | `AudioProcessor` |
| Namespaces | `CamelCase` | `Loudness` |
| Functions | `camelBack` | `processBlock` |
| Variables | `camelBack` | `sampleRate` |
| Parameters | `camelBack` | `numSamples` |
| Members | `camelBack` | `bufferSize` |
| Constants | `camelBack` | `defaultValue` |

## Array Usage

**Use `std::array` with `.at()` in UI/non-critical code:**
- Provides bounds checking at runtime
- Satisfies `cppcoreguidelines-pro-bounds-constant-array-index` clang-tidy check
- Acceptable overhead for code not in tight loops

**Use C-style arrays with `// NOLINTNEXTLINE` in audio/performance-critical code:**
- Audio callbacks run at sample rate (44,100+ Hz) where `.at()` overhead matters
- Add `// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)` above the line
- Ensure bounds are validated by design (loop bounds, index resets, etc.)

## Linting

**After making code changes, run linting to catch issues:**

```bash
# Lint all source files (requires compile_commands.json)
./scripts/lint.sh

# Lint specific files
./scripts/lint.sh Apps/mbk/Source/Main.cpp

# Specify build directory with compile_commands.json
./scripts/lint.sh -b cmake-build-release

# Auto-fix issues
./scripts/lint.sh --fix

# Check formatting only
./scripts/lint.sh --format

# Fix formatting
./scripts/lint.sh --format-fix
```

Uses clang-tidy with checks configured in `.clang-tidy`. Requires `compile_commands.json`.

**compile_commands.json Locations:**

| Location | How Created | Notes |
|----------|-------------|-------|
| `out/build/x64-Debug/` | VS Code CMake Tools | Auto-generated, recommended |
| `cmake-build-release/` | CLion | Windows paths |

**Why compile_commands.json Cannot Be Generated Elsewhere:**
- `build-windows/` - Visual Studio generator doesn't support `CMAKE_EXPORT_COMPILE_COMMANDS`
- Native Linux build - Fails because JUCE requires X11 headers (`apt install libx11-dev` would fix, but unnecessary)
- Ninja from WSL - Can't find MSVC compilers without VS Developer Command Prompt environment

**WSL Linting Setup:**

Use `run_clang_tidy.sh` which handles WSL-to-Windows path conversion:

```bash
# Ensure compile_commands.json exists (VS Code CMake Tools creates it, or configure in CLion)
# Run clang-tidy via helper script
./scripts/run_clang_tidy.sh
```

Note: `lint.sh` doesn't work from WSL because it passes Linux paths to clang-tidy, which can't match Windows paths in `compile_commands.json`.

**Exclusions:**
- `*/Libs/*` - Third-party libraries are excluded from linting
- `*Test.cpp` - Test files are excluded from linting

**IDE Integration:**
- VS Code: Install "clangd" extension, it uses `.clang-tidy` automatically
- CLion: Built-in clang-tidy support, enable in Settings > Editor > Inspections
- Visual Studio: Built-in since VS 2019, enable in Project Properties > Code Analysis

## Logging

The mbk app uses a custom logger (`logger::Logger`) defined in `Apps/mbk/Source/Logger/Logger.h`.

**Interface:**
```cpp
logger.debug("Debug message");  // Development/debugging info
logger.info("Info message");    // Normal operational messages
logger.error("Error message");  // Error conditions
```

**Usage pattern:**
1. Add `logger::Logger&` parameter to constructor
2. Store as member: `logger::Logger& logger;`
3. Initialize in constructor: `: logger(loggerRef)`
4. Call methods: `logger.debug("Value: " + String(value));`

**Example:**
```cpp
class MyComponent : public juce::Component {
public:
    explicit MyComponent(logger::Logger& loggerRef)
        : logger(loggerRef) {
        logger.info("MyComponent created");
    }

private:
    logger::Logger& logger;
};
```

## Workflow

After any code changes:
1. Build to verify compilation (from WSL):
   ```bash
   ./scripts/cmake.sh --build build-windows --config Debug --target mbk
   ```
2. Run linting: `./scripts/run_clang_tidy.sh`
3. Run tests:
   ```bash
   ./scripts/cmake.sh --build build-windows --config Debug --target UnitTestRunner
   ./build-windows/Tests/UnitTestRunner_artefacts/Debug/UnitTestRunner.exe
   ```
4. Auto-fix formatting before committing:
   ```bash
   ./scripts/lint.sh --format-fix
   ```
5. Fix any remaining issues before committing
