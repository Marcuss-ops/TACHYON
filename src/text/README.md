# Text Module

Complete text rendering subsystem with Unicode support, shaping, layout, and animation.

## Current Model

The text stack is split into authoring, runtime animation, and rendering:

- authoring code builds text layers and text scene presets
- preset registries provide reusable text layer and animator names
- runtime text code handles shaping, layout, and rasterization
- renderer2d consumes the evaluated text layer

## Directory Structure

```
src/text/
├── bidi/              # Bi-directional text support (RTL/LTR)
├── core/              # Core text functionality
│   ├── animation/     # Text animation utilities
│   ├── fonts/         # Font loading, caching, and management
│   ├── layout/        # Text layout engine
│   ├── rendering/     # Text rasterization
│   └── content/       # Subtitles and text content
├── editing/           # Text editor components
└── i18n/             # Internationalization support
```

## Key Extension Points

- `include/tachyon/text/animation/text_scene_presets.h`
- `src/text/core/animation/text_scene_presets.cpp`
- `include/tachyon/presets/text/text_builders.h`
- `src/presets/text/text_builders.cpp`
- `include/tachyon/presets/text/text_animator_preset_registry.h`
- `src/presets/text/text_animator_preset_registry.cpp`
- `src/text/core/animation/text_presets.cpp`

## How to Add a Text Feature

1. Add the new authoring shape in the existing text builder layer.
2. Add a preset registry entry only if the feature should be reusable by name.
3. Add runtime animation or layout support in the text core path.
4. Keep renderer changes minimal and local to the existing text rendering path.
5. Add focused tests close to the feature.

## Rules

- Do not create a second text pipeline.
- Do not move layout logic into renderer code.
- Do not add hardcoded animation catalogs in unrelated files.
- Do not change the scene contract just to add a new text preset name.

## Dependencies

- FreeType: font rasterization
- HarfBuzz: text shaping
- Unicode data: character properties
