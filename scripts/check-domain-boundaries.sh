#!/bin/bash
# Tachyon Domain Boundary Checker
# Enforces strict separation between engine domains.

EXIT_CODE=0

echo "--- Checking Tachyon Domain Boundaries ---"

# 1. Renderer2D must not depend on Scene3D internals
echo "Checking Renderer2D -> Scene3D boundaries..."
VIOLATIONS=$(grep -r "tachyon/scene3d" src/renderer2d | grep -v "scene3d_bridge")
if [ ! -z "$VIOLATIONS" ]; then
    echo "CRITICAL: Renderer2D violates Scene3D boundary!"
    echo "$VIOLATIONS"
    EXIT_CODE=1
fi

# 2. Scene3D must not depend on Timeline internals
echo "Checking Scene3D -> Timeline boundaries..."
if [ -d "src/scene3d" ]; then
    VIOLATIONS=$(grep -r "tachyon/timeline" src/scene3d)
    if [ ! -z "$VIOLATIONS" ]; then
        echo "CRITICAL: Scene3D violates Timeline boundary!"
        echo "$VIOLATIONS"
        EXIT_CODE=1
    fi
fi

# 3. Text must not depend on Renderer2D internals (except for public bridges if any)
echo "Checking Text -> Renderer2D boundaries..."
VIOLATIONS=$(grep -r "tachyon/renderer2d" src/text | grep -v "low_level")
if [ ! -z "$VIOLATIONS" ]; then
    echo "CRITICAL: Text violates Renderer2D boundary!"
    echo "$VIOLATIONS"
    EXIT_CODE=1
fi

# 4. Core Spec must be leaf (no dependencies on runtime or other heavy domains)
echo "Checking Core Spec isolation..."
VIOLATIONS=$(grep -r "tachyon/renderer2d\|tachyon/scene3d\|tachyon/runtime" include/tachyon/core/spec)
if [ ! -z "$VIOLATIONS" ]; then
    echo "CRITICAL: Core Spec depends on runtime or renderer!"
    echo "$VIOLATIONS"
    EXIT_CODE=1
fi

if [ $EXIT_CODE -eq 0 ]; then
    echo "SUCCESS: Domain boundaries are intact."
else
    echo "FAILURE: Domain boundary violations detected."
fi

exit $EXIT_CODE
