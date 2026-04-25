# Text Module

Complete text rendering subsystem with Unicode support, shaping, layout, and animation.

## Directory Structure

```
src/text/
├── bidi/              # Bi-directional text support (RTL/LTR)
├── core/              # Core text functionality
│   ├── animation/     # Text animation utilities
│   ├── fonts/        # Font loading, caching, and management
│   ├── layout/       # Text layout engine
│   ├── rendering/    # Text rasterization
│   └── content/      # Subtitles and text content
├── editing/           # Text editor components
└── i18n/             # Internationalization support
```

## Key Components

### Font System (`core/fonts/`)
- `font.cpp`: Main font class with FreeType integration
- `font_registry.cpp`: Global font registry
- `font_face.cpp`: Font face management
- `font_instance.cpp`: Instantiated font at specific sizes
- `glyph_loader.cpp`: Glyph loading and caching

### Layout Engine (`core/layout/`)
- `layout.cpp`: Main layout interface
- `layout_engine.cpp`: Line breaking, alignment, justification
- `layout_cache.cpp`: Cached layout results

### Animation (`core/animation/`)
- `text_animator_utils.cpp`: Per-glyph animation (typewriter, wave, etc.)
- `text_animator_pipeline.cpp`: Animation pipeline

### Rendering (`core/rendering/`)
- `text_raster_surface.cpp`: Surface-based text rendering
- `outline_extractor.cpp`: Vector outline extraction

## Dependencies
- FreeType: Font rasterization
- HarfBuzz: Text shaping
- ICU/Unicode: Character properties
