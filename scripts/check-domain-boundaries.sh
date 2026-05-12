#!/bin/bash
# Tachyon Domain Boundary Checker
# Enforces strict separation between engine domains.

EXIT_CODE=0

echo "--- Checking Tachyon Domain Boundaries ---"

# 1. Text must not depend on Renderer2D internals (except for public bridges if any)
echo "Checking Text -> Renderer2D boundaries..."
VIOLATIONS=$(grep -r "tachyon/renderer2d" src/text | grep -v "low_level")
if [ ! -z "$VIOLATIONS" ]; then
    echo "CRITICAL: Text violates Renderer2D boundary!"
    echo "$VIOLATIONS"
    EXIT_CODE=1
fi

# 2. Core Spec must be leaf (no dependencies on runtime or other heavy domains)
echo "Checking Core Spec isolation..."
VIOLATIONS=$(grep -r "tachyon/renderer2d\|tachyon/runtime" include/tachyon/core/spec)
if [ ! -z "$VIOLATIONS" ]; then
    echo "CRITICAL: Core Spec depends on runtime or renderer!"
    echo "$VIOLATIONS"
    EXIT_CODE=1
fi

# 3. Core Scene must not depend on render intent headers
echo "Checking Core Scene -> Render boundaries..."
VIOLATIONS=$(grep -r "tachyon/render/" include/tachyon/core/scene src/core/scene | grep -E "^\s*#include")
if [ ! -z "$VIOLATIONS" ]; then
    echo "CRITICAL: Core Scene depends on render intent headers!"
    echo "$VIOLATIONS"
    EXIT_CODE=1
fi

if [ $EXIT_CODE -eq 0 ]; then
    echo "SUCCESS: Domain boundaries are intact."
else
    echo "FAILURE: Domain boundary violations detected."
fi

exit $EXIT_CODE
