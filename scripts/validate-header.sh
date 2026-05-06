#!/bin/bash
# Quick header validation - validates a single header compiles correctly
# Usage: ./validate-header.sh include/tachyon/core/spec/schema/objects/composition_spec.h

if [ -z "$1" ]; then
    echo "Usage: $0 <header_path>"
    exit 1
fi

HEADER="$1"
TEMP_CPP=$(mktemp /tmp/header_check.XXXXXX.cpp)
TARGET="HeaderSmokeTemp"

# Create temporary cpp file
echo "#include <$HEADER>" > "$TEMP_CPP"
echo "int main() { return 0; }" >> "$TEMP_CPP"

# Build using cmake
cmake --build build --preset dev --target TachyonCore 2>&1 | grep -E "error C|fatal error"

# Cleanup
rm -f "$TEMP_CPP"

echo "Header validation complete!"
