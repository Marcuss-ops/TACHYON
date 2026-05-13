---
Status: Canonical
Last reviewed: 2026-05-06
Owner: Core Team
Supersedes: N/A
---

# Universal LayerSurface Rule

Tachyon's reusable spatial composition pipeline is split into two stages.

1. Source-to-surface
   - image decode
   - video frame sampling
   - SVG rasterization
   - text layout and rasterization
   - shape rasterization
   - precomp rendering

   This is the only stage where layer-type branching is allowed.

2. Surface-to-Spatial
   - `LayerSurface` promotion to a textured spatial plane
   - `SpatialTransform`
   - camera
   - lights
   - compositing

   This stage must be source-agnostic.

Rules:
- Source-specific logic ends at `LayerSurface`.
- Spatial logic starts at `LayerSurface`.
- No source-type checks are allowed inside the spatial bridge.
- The bridge may only reason about shared rendering data, transforms, and compositing state.
- True extrusion or mesh-generation paths stay separate from the universal plane path.

The intent is to keep one reusable spatial contract for every renderable layer without adding per-type wrapper code.
