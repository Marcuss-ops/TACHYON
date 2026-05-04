# Backgrounds in TACHYON

This document describes the architecture of the background system and the rules for extending it.

## Architecture

The background system is divided into three layers:

1.  **BackgroundSpec (Schema)**: A thin wrapper around a string that detects whether it's a color, a component reference, an asset, or a preset. It does NOT contain a list of valid presets.
2.  **BackgroundPresetRegistry (Catalog)**: The single source of truth for all background presets. It contains a catalog of `BackgroundParams` mapped to preset IDs.
3.  **BackgroundBuilders (Factory)**: A parametric factory that takes `BackgroundParams` and produces a `LayerSpec` using procedural generation.

## How to add a new Background Preset

To add a new preset, follow these steps:

1.  Define the `BackgroundParams` for your preset.
2.  Add an entry to the `get_preset_defs()` list in `src/presets/background/background_preset_registry.cpp`.

```cpp
{"my_new_preset", "My New Preset", {
    .kind = "aura", 
    .palette = "neon_night",
    .speed = 0.5f,
    .frequency = 2.0f
}}
```

3.  The preset will automatically be available via `list_background_presets()` and `build_background_preset()`.

## Rules

- **Do NOT** add hardcoded preset lists to `BackgroundSpec`.
- **Do NOT** duplicate building logic in the registry. Always use `build_background(params)`.
- **Prefer** using existing palettes when possible.
- **Always** provide a human-readable name for the preset.

## Background Specification Syntax

- `#RRGGBB` / `rgb(...)` -> Color
- `preset:xxx` -> Preset reference
- `component:xxx` -> Component reference
- `path/to/image.png` -> Asset reference
- `xxx` (fallback) -> Component reference (most common case)
