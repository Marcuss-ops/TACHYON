# 2D Renderer Module

2D rendering pipeline for the Tachyon engine, handling compositing, effects, and text rendering.

## Directory Structure

```
src/renderer2d/
├── backend/         # Rendering backends (CPU, Vulkan)
├── color/           # Color management and blending
├── core/            # Core 2D rendering primitives
├── effects/         # Visual effects (blur, particles, etc.)
├── evaluated_composition/  # Evaluated composition rendering
│   ├── rendering/   # Layer renderers
│   ├── effects/     # Effect rendering
│   ├── matte/       # Matte resolution
│   └── utilities/   # Composition utilities
├── raster/          # Rasterization pipeline
│   └── path/        # Path rasterization
├── resource/        # Resource management (textures, caches)
├── spec/            # 2D-specific specifications
└── text/            # Text rendering subsystem
    ├── freetype/     # FreeType integration
    ├── glyph/        # Glyph loading and rendering
    ├── shaping/      # Text shaping with HarfBuzz
    ├── utf8/         # UTF-8 decoding
    └── utils/        # Text utilities
```

## Key Components

- **backend/cpu_backend.cpp**: CPU-based rendering backend
- **backend/vulkan_backend.cpp**: Vulkan GPU backend
- **color/blending.cpp**: Blend mode implementations
- **effects/effect_host.cpp**: Effect system host
- **raster/draw_list_builder.cpp**: Draw list construction

## Submodules

### Effects (`effects/`)
- `blur_effects.cpp`: Gaussian, box, directional blur
- `color_effects.cpp`: Color correction effects
- `particle_effects.cpp`: Particle system effects
- `glsl_transition_effect.cpp`: GLSL-based transitions

### Text Rendering (`text/`)
Integrated with the main `text/` module but with 2D-specific rendering.

## Note
The `audio/` subdirectory previously located here has been moved to the top-level `src/audio/` module for better separation of concerns.
