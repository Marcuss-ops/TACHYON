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
















## Current Implementation Status

| Component / Requirement | Status | Files | Notes |
|---|---|---|---|
| **`EvaluatedScene3D`** formalization | ✅ | `include/tachyon/core/render/scene_3d.h` | Canonical unified 3D scene model. |
| **`IRayTracer`** interface | ✅ | `include/tachyon/core/render/iray_tracer.h` | Strict contract for bridge clients. |
| **AOV Buffer** for beauty/depth | ✅ | `include/tachyon/core/render/aov_buffer.h` | Single-allocation storage for hybrid compositing. |
| **`composition_renderer.cpp`** isolation | ✅ | `src/tachyon/core/render/composition_renderer.cpp` | Forward declarations only; no `renderer3d` headers. |
| **Concrete `RayTracer`** injection | ✅ | `src/tachyon/core/render/frame_executor.cpp` | Gated via `#ifdef TACHYON_ENABLE_3D` for both standard and motion-blur paths. |
| **Hybrid 2D/3D compositing** | ✅ | `src/tachyon/core/render/frame_executor.cpp`, `src/tachyon/core/render/composition_renderer.cpp` | 3D blocks at index; 2D layers use depth buffer when `world_position3.z` is set. |
| **Fallback 2D card** when 3D unavailable | ✅ | `src/tachyon/core/render/frame_executor.cpp` | When `!TACHYON_ENABLE_3D` or tracer fails, renders a 2D screen-space projection of the 3D layer card. |
| **`renderer3d` headers excluded** from 2D | ✅ | Header dependency analysis complete | No `#include` of `renderer3d/*` in `core/render` or `renderer2d/*`. |
| **Runtime 3D-layer detection** | ✅ | `evaluate_layer` pipeline | Analyzes `layer->render_input.is_3d` at evaluation time to decide whether to construct `EvaluatedScene3D`. |

## Verification Strategy

### Unit Tests

| Test Suite | Coverage | Location |
|---|---|---|
| **`scene_3d_test.cpp`** | ✅ 35 tests | `tests/unit/render/scene_3d_test.cpp` |
| **`aov_buffer_test.cpp`** | ✅ 24 tests | `tests/unit/render/aov_buffer_test.cpp` |
| **`composition_renderer_test.cpp`** | ✅ 19 tests | `tests/unit/render/composition_renderer_test.cpp` |
| **`frame_executor_test.cpp`** (hybrid compositing) | ✅ Partial (16 tests) | `tests/unit/render/frame_executor_test.cpp` |
| **Total** | ✅ **~98 tests** | | 

### Integration Tests

| Test Suite | Coverage | Location |
|---|---|---|
| **`three_d_validation_lab.cpp`** | ✅ Full pipeline | `tests/perf/three_d_validation_lab.cpp` |
| **`transition_perf_lab.cpp`** | ✅ 3D-layer aware | `tests/perf/transition_perf_lab.cpp` |

### Automated Verification Commands

```bash
# Run all tests
cmake --build build --target test
ctest --output-on-failure --schedule-random

# Verify coverage (when coverage enabled)
gcovr -r . --filter src/tachyon/core --filter src/tachyon/renderer2d --filter src/tachyon/renderer3d --filter src/tachyon/ui --exclude-pattern "*test*" --exclude-pattern "*/third_party/*" --html --html-details -o coverage.html

# Static analysis (Clang-Tidy)
cmake --build build --target tachyon_lint
```
