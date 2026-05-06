# Tachyon 2D/3D Bridge Contract

The 2D/3D Bridge is the formal interface between the 2D compositing domain and the 3D rendering domain. It allows the 2D engine to consume 3D blocks as high-fidelity surfaces with optional depth/AOV data.

## Architectural Goal

The 3D domain functions as a **producer** of surfaces that the 2D domain **consumes** at the appropriate stack index. Effects (blurs, color corrections, transitions) can be applied to 3D output just like any other 2D layer.

## Implementation Status: COMPLETE

| Requirement | Status |
|---|---|
| `IRayTracer` interface contract | ✅ `include/tachyon/core/render/iray_tracer.h` |
| `EvaluatedScene3D` as unified bridge type | ✅ `include/tachyon/core/render/scene_3d.h` |
| AOV buffer for beauty/depth transfer | ✅ `include/tachyon/core/render/aov_buffer.h` |
| `composition_renderer.cpp` free of `renderer3d` headers | ✅ |
| Concrete `RayTracer` instantiation in `frame_executor.cpp` | ✅ (both motion-blur and standard paths) |
| Fallback 2D card when 3D renderer unavailable | ✅ |

## Data Flow Contract

### 1. Block Identification

The 2D renderer identifies contiguous blocks of 3D layers. A "block" starts at the first layer with `is_3d = true` and ends when a 2D or adjustment layer is encountered.

### 2. Bridge Input (`Scene3DBridgeInput`)

Built from a subset of `EvaluatedCompositionState`:
- **Camera State:** current camera projection and view matrices.
- **Layer Subset:** `EvaluatedLayerState` instances belonging to the 3D block.
- **Context:** `RenderContext2D` holding the injected `shared_ptr<IRayTracer>`.

### 3. Bridge Output

- **`EvaluatedScene3D`:** handed directly to `IRayTracer::build_scene`.
- **`AOVBuffer`:** filled by `IRayTracer::render` (beauty RGBA + linear depth Z).
- **`SurfaceRGBA`:** assembled from AOV data and composited at the correct stack position.

### 4. Injection Point

The concrete `renderer3d::RayTracer` is created in `frame_executor.cpp` inside `#ifdef TACHYON_ENABLE_3D`, stored as `shared_ptr<IRayTracer>` on `RenderContext2D`.
`composition_renderer.cpp` calls methods exclusively via the `IRayTracer` interface — it has no direct dependency on `renderer3d`.

## Compositing Strategy

- **Z-Ordering:** 3D blocks are composited at their respective index in the layer stack.
- **Hybrid Intersection:** 2D layers with `world_position3.z` set use the 3D depth buffer for Z-tested compositing.
- **Fallbacks:** If `TACHYON_ENABLE_3D` is off or the ray tracer produces no visible pixels, a "2D card" preview is rendered from the layer's world matrix projected into 2D screen space.

## Strict Isolation Rules

1. **No Leakage:** 2D core headers use forward declarations and `IRayTracer` only. No `#include` of `renderer3d` headers inside `renderer2d` or `core`.
2. **On-Demand Allocation:** `RayTracer` and Embree BVH are only allocated when a 3D block is present in the composition.
3. **`TACHYON_ENABLE_3D` guard:** All concrete 3D types are gated behind this preprocessor flag at every injection site.
