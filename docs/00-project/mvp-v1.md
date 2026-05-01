# MVP v1 Scope

## MVP Definition

The Minimum Viable Product (v1) for Tachyon focuses on core rendering capabilities with a programmatic C++ API.

## Core Features (v1)

### Scene Authoring
- C++ Builder API for type-safe scene construction
- Basic scene graph with layers and properties
- Camera and 3D mesh support
- Material system (metallic, roughness, etc.)

### 2D Compositing
- Shape layers (rectangles, circles, paths)
- Text rendering with font support
- Basic effects (blur, color correction)
- Layer masking and mattes
- Motion blur

### 3D Rendering
- CPU-based path tracing (simplified)
- Camera models (perspective, orthographic)
- Basic lighting (directional, point)
- Mesh loading (glTF format)

### Output
- Video encoding integration (FFmpeg)
- Deterministic frame output
- Basic quality tiers

## Out of Scope (v1)

- Advanced 3D features (complex materials, global illumination)
- Audio reactivity
- Timeline editing GUI
- Browser-based preview
- Complex particle systems
- Advanced text layout (RTL, vertical text)

## Success Criteria

- Can render a basic 3D product promo video from C++ code
- Deterministic output (same code = same video)
- Clean C++ Builder API
- Basic test coverage for core features
