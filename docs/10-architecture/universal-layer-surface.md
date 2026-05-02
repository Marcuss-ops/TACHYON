# Universal LayerSurface Rule

Tachyon's reusable 2.5D/3D pipeline is split into two stages.

1. Source-to-surface
   - image decode
   - video frame sampling
   - SVG rasterization
   - text layout and rasterization
   - shape rasterization
   - precomp rendering

   This is the only stage where layer-type branching is allowed.

2. Surface-to-3D
   - `LayerSurface` promotion to a textured 3D plane
   - `Transform3D`
   - camera
   - lights
   - compositing

   This stage must be source-agnostic.

Rules:
- Source-specific logic ends at `LayerSurface`.
- 3D-specific logic starts at `LayerSurface`.
- No source-type checks are allowed inside the 3D bridge.
- The bridge may only reason about shared rendering data, transforms, and compositing state.
- True extrusion or mesh-generation paths stay separate from the universal plane path.

The intent is to keep one reusable 3D contract for every renderable layer without adding per-type wrapper code.
