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
        --dev)             BASE_PRESET="dev"; BUILD_TYPE="RelWithDebInfo" ;;
        --dev-fast)        BASE_PRESET="dev-fast"; BUILD_TYPE="RelWithDebInfo" ;;
        --test)            RUN_TESTS=1 ;;
        --clean)           CLEAN=1 ;;
        --check)           CHECK=1 ;;
        -h|--help)         usage ;;
        *) echo "Unknown argument: $arg"; usage ;;
    esac
done

# Select final preset
if [[ "$BASE_PRESET" == "dev" ]]; then
    PRESET="dev-linux"
elif [[ "$BASE_PRESET" == "dev-fast" ]]; then
    PRESET="dev-linux-fast"
elif [[ "$BASE_PRESET" == "release" ]]; then
    PRESET="release"
elif [[ "$BASE_PRESET" == "debug" ]]; then
    PRESET="dev-linux" # Fallback for debug if not present
else
    PRESET="$BASE_PRESET"
fi

# Map preset to binary directory (should match CMakePresets.json)
BUILD_DIR="build/$PRESET"

if [[ $CLEAN -eq 1 && -d "$BUILD_DIR" ]]; then
    echo "[INFO] Cleaning build directory $BUILD_DIR..."
    rm -rf "$BUILD_DIR"
fi

EMBREE_DIR="$PWD/third_party/embree"
if [ ! -d "$EMBREE_DIR" ]; then
    echo "[INFO] Downloading precompiled Embree 3.13.5..."
    mkdir -p "$EMBREE_DIR"
    curl -L https://github.com/embree/embree/releases/download/v3.13.5/embree-3.13.5.x86_64.linux.tar.gz | tar -xz -C "$EMBREE_DIR" --strip-components=1
fi

echo "[INFO] Configuring with preset: $PRESET (Generator: $GENERATOR_TYPE)"
cmake --preset "$PRESET" -DTACHYON_EMBREE_ROOT="$EMBREE_DIR"

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
    "$BUILD_DIR/tests/TachyonTests"
    echo "[OK] All tests passed"
    exit 0
fi

echo "[INFO] Building (type: $BUILD_TYPE)..."
cmake --build "$BUILD_DIR" -- -j"$(nproc)"
echo "[OK] Build completed successfully"
