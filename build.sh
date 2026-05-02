#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE="Release"
BASE_PRESET="release"
RUN_TESTS=0
CLEAN=0
CHECK=0

usage() {
    echo "Usage: $0 [--debug|--release|--relwithdebinfo] [--test] [--clean] [--check]"
    echo "  --debug           Debug build"
    echo "  --release         Release build (default)"
    echo "  --relwithdebinfo  Release with debug info"
    echo "  --test            Build and run tests"
    echo "  --clean           Delete build dir and reconfigure"
    echo "  --check           Build only HeaderSmokeTests"
    exit 1
}

# Check for required tools
if ! command -v cmake &> /dev/null; then
    echo "[ERROR] cmake is not installed or not in PATH"
    exit 1
fi

GENERATOR_TYPE="make"
if command -v ninja &> /dev/null; then
    GENERATOR_TYPE="ninja"
elif command -v make &> /dev/null; then
    GENERATOR_TYPE="make"
else
    echo "[ERROR] Neither ninja nor make found in PATH"
    exit 1
fi

for arg in "$@"; do
    case $arg in
        --debug)           BASE_PRESET="debug"; BUILD_TYPE="Debug" ;;
        --release)         BASE_PRESET="release"; BUILD_TYPE="Release" ;;
        --relwithdebinfo)  BASE_PRESET="relwithdebinfo"; BUILD_TYPE="RelWithDebInfo" ;;
        --test)            RUN_TESTS=1 ;;
        --clean)           CLEAN=1 ;;
        --check)           CHECK=1 ;;
        -h|--help)         usage ;;
        *) echo "Unknown argument: $arg"; usage ;;
    esac
done

# Select final preset based on generator
PRESET="linux-$GENERATOR_TYPE-$BASE_PRESET"

# Map preset to binary directory (should match CMakePresets.json)
BUILD_DIR="build/$PRESET"

if [[ $CLEAN -eq 1 && -d "$BUILD_DIR" ]]; then
    echo "[INFO] Cleaning build directory $BUILD_DIR..."
    rm -rf "$BUILD_DIR"
fi

echo "[INFO] Configuring with preset: $PRESET (Generator: $GENERATOR_TYPE)"
cmake --preset "$PRESET"

if [[ $CHECK -eq 1 ]]; then
    echo "[INFO] Running syntax check..."
    cmake --build "$BUILD_DIR" --target HeaderSmokeTests
    echo "[OK] Syntax check passed"
    exit 0
fi

if [[ $RUN_TESTS -eq 1 ]]; then
    echo "[INFO] Building tests..."
    cmake --build "$BUILD_DIR" --target TachyonTests
    echo "[INFO] Running tests..."
    "$BUILD_DIR/tests/unit/TachyonTests"
    echo "[OK] All tests passed"
    exit 0
fi

echo "[INFO] Building (type: $BUILD_TYPE)..."
cmake --build "$BUILD_DIR" -- -j"$(nproc)"
echo "[OK] Build completed successfully"
