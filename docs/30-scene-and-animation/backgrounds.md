# Backgrounds

This document describes how backgrounds work today and where to extend the system without changing the rendering logic.

## Current State

- `BackgroundPresetRegistry` is intentionally empty by default.
- `BackgroundKindRegistry` is intentionally empty by default.
- `build_background()` is still the single entry point for turning `BackgroundParams` into a `LayerSpec`.
- There is no implicit fallback visual. If the caller does not request a background, the builder returns a disabled layer.

## Data Flow

1. A caller fills `BackgroundParams`.
2. `build_background()` decides whether the request is explicit or empty.
3. If `kind` is set, the builder preserves that id in the layer.
4. If a matching kind factory exists, it fills `procedural`.
5. If no matching factory exists, the requested kind is still preserved as the authoring intent.

## Extension Points

- `include/tachyon/presets/background/background_params.h`
- `src/presets/background/background_builders.cpp`
- `src/presets/background/background_kind_table.cpp`
- `src/presets/background/background_presets_table.cpp`
- `include/tachyon/presets/background/procedural.h`

## How to Add a Background

1. Define or reuse a procedural helper in `include/tachyon/presets/background/procedural.h`.
2. Register the kind in `src/presets/background/background_kind_table.cpp` if you want a renderable procedural variant.
3. Register a preset in `src/presets/background/background_presets_table.cpp` if you want a named authoring shortcut.
4. Pass the explicit `BackgroundParams.kind` from the caller.
5. Keep `build_background()` as the only builder path.

## Rules

- Do not add hidden default backgrounds.
- Do not add duplicate factory logic in the registry.
- Do not make `BackgroundSpec` own the preset catalog.
- Do not change the rendering path just to add a new background name.

## Practical Result

- Empty `kind` means no procedural background request.
- A named `kind` means the scene author asked for that background explicitly.
- The registry remains the source of optional mappings, not the source of implicit behavior.
