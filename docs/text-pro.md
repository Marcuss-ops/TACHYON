# Text Pro Integration

Tachyon includes an expanded text rendering pipeline ("Text Pro") with fallback fonts, basic bidirectional handling, HarfBuzz shaping hooks, and caching.

## Key Features

- **Font Fallback Chain**: The registry can return a fallback chain for a font name, and the layout engine will choose the first font in the chain that covers each codepoint.
- **BiDi (Bidirectional) Text**: The pipeline splits text into simple LTR/RTL runs.
- **HarfBuzz Script/Language Shaping**: The layout engine detects a coarse script/language pair for each run and passes it to HarfBuzz.
- **Shaping Cache**: A thread-safe `ShapingCache` stores shaped glyph runs by font, scale, direction, script, language, and text content.
- **Layout Features**:
  - **Justification**: Supported in the 2D text layout path.
  - **Tabs**: Tab characters are handled during layout.
  - **Wrapping**: Word wrapping is present in the layout engine, but it is still pragmatic rather than fully typographic.
- **2D Engine Integration**: Text layers render through the 2D compositing pipeline, with camera projection for 3D layers and highlight spans.
- **3D Engine Integration**: The ray tracer can extrude text and use a `FontRegistry` when one is provided.

## Usage

### 2D Rendering
In the scene specification, set a layer type to `text`. The 2D renderer uses the `FontRegistry` provided in `RenderContext2D` for layout and glyph rasterization.

### 3D Rendering
Text layers marked as `is_3d: true` can be extruded in the ray tracer. The ray tracer accepts a `FontRegistry` so it can reuse the same font selection path as the 2D engine.

## Implementation Details

- **`InternalLayoutEngine`**: A unified layout engine that handles bidi analysis, fallback selection, and shaping.
- **`ScriptDetector`**: Detects a coarse script/language pair from Unicode ranges.
- **`ShapingCache`**: Thread-safe cache keyed by font ID, scale, direction, script, language, and codepoints.
- **`BidiEngine`**: Splits text into directional runs using a simplified heuristic.
