#!/bin/bash
# Pre-commit hook for Tachyon project
# Runs quick checks before allowing commit

set -e

echo "Running pre-commit checks..."

# Run header smoke tests
echo "Checking header smoke tests..."
cmake --build build-ninja --preset relwithdebinfo --target HeaderSmokeTests 2>&1 | grep -E "error C|fatal error|FAILED" && exit 1

# Run core build check
echo "Running core build check..."
cmake --build build-ninja --preset relwithdebinfo --target TachyonCore 2>&1 | grep -E "error C|fatal error|FAILED" && exit 1

echo "Pre-commit checks passed!"
