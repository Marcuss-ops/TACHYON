#!/usr/bin/env bash
set -euo pipefail

echo "[Tachyon] Starting fast developer inner-loop compilation and test run..."

# Configure and compile using fast preset
cmake --preset dev-linux-fast
cmake --build --preset dev-linux-fast -j"$(nproc)"

# Run unit tests only, excluding long-running golden comparisons and renders
ctest --test-dir build/dev-linux-fast --output-on-failure --label-exclude "render|golden"
