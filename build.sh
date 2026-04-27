#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE="Release"
PRESET="linux-release"
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

for arg in "$@"; do
    case $arg in
        --debug)           PRESET="linux-debug"; BUILD_TYPE="Debug" ;;
        --release)         PRESET="linux-release"; BUILD_TYPE="Release" ;;
        --relwithdebinfo)  PRESET="linux-relwithdebinfo"; BUILD_TYPE="RelWithDebInfo" ;;
        --test)            RUN_TESTS=1 ;;
        --clean)           CLEAN=1 ;;
        --check)           CHECK=1 ;;
        -h|--help)         usage ;;
        *) echo "Unknown argument: $arg"; usage ;;
    esac
done

BUILD_DIR="build-ninja"

if [[ $CLEAN -eq 1 && -d "$BUILD_DIR" ]]; then
    echo "[INFO] Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

echo "[INFO] Configuring with preset: $PRESET"
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
cmake --build --preset "$PRESET" -- -j"$(nproc)"
echo "[OK] Build completed successfully"
