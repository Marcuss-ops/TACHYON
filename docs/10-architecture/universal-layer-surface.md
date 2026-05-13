---
Status: Canonical
Last reviewed: 2026-05-06
Owner: Core Team
Supersedes: N/A
---

# Universal LayerSurface Rule

Tachyon's reusable 2D compositing pipeline is split into two stages.

1. Source-to-surface
   - image decode
   - video frame sampling
   - SVG rasterization
   - text layout and rasterization
   - shape rasterization
   - precomp rendering

   This is the only stage where layer-type branching is allowed.

2. Surface-to-Compositor
   - `LayerSurface` promotion to a renderable surface
   - Transform application (Position, Scale, Rotation)
   - Alpha blending
   - Compositing

   This stage must be source-agnostic.

Rules:
- Source-specific logic ends at `LayerSurface`.
- Compositing logic starts at `LayerSurface`.
- No source-type checks are allowed inside the compositor bridge.
- The bridge may only reason about shared rendering data, transforms, and compositing state.

The intent is to keep one reusable contract for every renderable layer without adding per-type wrapper code.
