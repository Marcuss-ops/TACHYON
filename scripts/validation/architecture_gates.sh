#!/bin/bash
set -e

echo "Running Tachyon Architecture Gates..."

# 1. LayerSpec legacy fields
echo "Checking LayerSpec for legacy fields..."
git grep -n "type_string" include/tachyon/core/spec/schema/objects/layer_spec.h && (echo "FAIL: type_string found in LayerSpec"; exit 1)
git grep -n "start_time" include/tachyon/core/spec/schema/objects/layer_spec.h && (echo "FAIL: start_time found in LayerSpec"; exit 1)
git grep -n "in_point" include/tachyon/core/spec/schema/objects/layer_spec.h && (echo "FAIL: in_point found in LayerSpec"; exit 1)
git grep -n "out_point" include/tachyon/core/spec/schema/objects/layer_spec.h && (echo "FAIL: out_point found in LayerSpec"; exit 1)
git grep -n "std::optional<double> duration" include/tachyon/core/spec/schema/objects/layer_spec.h && (echo "FAIL: duration found in LayerSpec"; exit 1)
echo "PASS: LayerSpec is clean."

# 2. TransitionSpec legacy type
echo "Checking LayerTransitionSpec for runtime-canonical type..."
# Runtime should use transition_id, not type.
# We check src/runtime and src/renderer2d
git grep -n "transition_in.type\|transition_out.type" src/runtime src/renderer2d && (echo "FAIL: transition type used in runtime"; exit 1)
echo "PASS: Runtime uses transition_id."

# 3. Effect model legacy maps
echo "Checking for legacy effect model maps (.scalars, .colors, .strings)..."
git grep -n "AnimatedEffectSpec" include src tests && (echo "FAIL: AnimatedEffectSpec found"; exit 1)
git grep -n "animated_effects" include src tests && (echo "FAIL: animated_effects found"; exit 1)
git grep -n "\.scalars\|\.colors\|\.strings" src/renderer2d src/core src/runtime include/tachyon | grep -v "evaluator" | grep -v "effect_params" && (echo "FAIL: legacy effect maps found"; exit 1)
echo "PASS: Effect model is unified."

# 4. Builder dependency on preset registries
echo "Checking core Builder for EffectPresetRegistry dependency..."
git grep -n "EffectPresetRegistry" include/tachyon/scene src/core/scene src/core/spec && (echo "FAIL: Builder depends on EffectPresetRegistry"; exit 1)
echo "PASS: Builder is registry-agnostic."

# 5. Singleton Registries in core/runtime
echo "Checking for singleton registries in core/runtime..."
# Whitelist known preset areas, but fail on core/runtime/renderer2d
git grep -n "Registry::instance" include src | grep -v "src/presets" | grep -v "include/tachyon/presets" | grep -v "src/library" | grep -v "src/runtime/execution/batch_runner" && (echo "FAIL: Singleton registry found in core/runtime"; exit 1)
echo "PASS: No singleton registries in critical core paths."

echo "All architecture gates PASSED."
