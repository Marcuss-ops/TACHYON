#!/bin/bash

# Tachyon Architectural Guardrails Check
# Ensures no legacy fields or banned patterns are reintroduced.

EXIT_CODE=0

echo "Running Tachyon Guardrails Check..."

# 1. Check for legacy Text fields in runtime/headers
BANNED_TEXT_FIELDS="per_glyph_offset_x|per_glyph_offset_y|per_glyph_scale_delta|per_glyph_opacity_drop|wave_amplitude_x|wave_amplitude_y|wave_period_seconds"
if grep -rE "$BANNED_TEXT_FIELDS" include/tachyon/text src/text --exclude-dir=animation/legacy; then
    echo "FAIL: Legacy Text fields found in restricted directories."
    EXIT_CODE=1
fi

# 2. Check for TextAnimatorPresetRegistry::instance() in core modules (should use explicit registries)
# We allow it in high-level presets/builders for now, but not in core/rendering
BANNED_SINGLETON_CORE="TextAnimatorPresetRegistry::instance()"
if grep -r "$BANNED_SINGLETON_CORE" src/text/core src/renderer2d src/renderer3d; then
    echo "FAIL: Singleton registry access found in core/renderer modules."
    EXIT_CODE=1
fi

# 3. Check for any 'instance()' in new domains
# (This is a bit broad, but helps focus review)
# if grep -r "::instance()" src/some_new_domain; then ... fi

if [ $EXIT_CODE -eq 0 ]; then
    echo "SUCCESS: All guardrails passed."
else
    echo "FAILURE: Guardrail violations detected. Please refer to docs/architecture/feature-extension-rules.md"
fi

exit $EXIT_CODE
