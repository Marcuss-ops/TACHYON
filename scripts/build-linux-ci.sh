#!/usr/bin/env bash
set -euo pipefail

echo "[Tachyon] Starting reproducible Linux CI compile and test pipeline..."

# Configure using system dependencies (enforcing no FetchContent downloads)
cmake --preset linux-container

# Build all configurations
cmake --build --preset linux-container -j"$(nproc)"

# Run all automated tests
ctest --test-dir build/linux-container --output-on-failure
