# MVP v1 Scope

## MVP Definition

The Minimum Viable Product (v1) for Tachyon focuses on core rendering capabilities with a programmatic C++ API. To prevent scope creep, features are strictly categorized into Production Ready, Experimental, Stubbed or Transitional, and Out of Scope.

## V1 Contract

### Production Ready

- C++ Builder API for type-safe scene construction
- Basic scene graph with layers and properties
- Shape layers (rectangles, circles, paths)
- Text rendering with font support
- Basic effects (blur, color correction)
- Layer masking and mattes
- Video encoding integration (FFmpeg)
- Deterministic frame output
- Basic quality tiers

### Experimental

- Camera and 3D mesh support
- Motion blur
- Basic 3D rendering path tracing
- Basic lighting and glTF mesh loading
- Material system (metallic, roughness, etc.)

### Stubbed or Transitional

- Audio reactivity
- Advanced 3D features
- Browser-based preview
- Complex particle systems
- Advanced text layout (RTL, vertical text)

### Out of Scope for v1

- Timeline editing GUI
- Feature families not needed for a deterministic C++-only baseline

## Success Criteria

- Can render a basic 3D product promo video from C++ code
- Deterministic output (same code = same video)
- Clean C++ Builder API
- Robust CI test coverage (Smoke, Core, Render tiers)
