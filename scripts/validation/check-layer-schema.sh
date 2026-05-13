#!/bin/bash
# check-layer-schema.sh
# Enforce LayerSpec type migration by failing if 'kind' is used.

FOUND_KIND=$(grep -rE "layer\.kind|\.kind\s*=|LayerSpec::kind" src include tests --include="*.cpp" --include="*.h" --include="*.hpp")

if [ -n "$FOUND_KIND" ]; then
    echo "ERROR: Legacy 'kind' usage found in LayerSpec schema!"
    echo "$FOUND_KIND"
    exit 1
fi

echo "SUCCESS: No legacy 'kind' usage found."
exit 0
