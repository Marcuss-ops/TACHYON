# Tachyon Optimization Plan

This document outlines a comprehensive strategy to optimize the Tachyon engine, moving beyond simple light leak improvements to address core architectural bottlenecks.

## Areas of Optimization

### 1. Render Batch and Frame Execution
The current batch runner limits each job to a single worker to avoid oversubscription. This should be replaced with a more intelligent policy.
*   **Strategy:** Implement a `resolve_per_job_workers` function that considers total machine threads, active jobs, resolution, and frame range.
*   **Features:** Frame lookahead queue, parallel rendering of frames (N, N+1, N+2), and a stable thread pool.
*   **Expected Gain:** 2x-8x.

### 2. Tile Rendering
Leverage the existing `TileGrid` infrastructure within the actual render loop, especially for 4K resolutions.
*   **Strategy:** Implement tile-based rasterization, compositing, and effects.
*   **Goal:** Move from full-frame processing to clipped tile processing.
*   **Expected Gain:** 2x-6x on 4K.

### 3. Static Layer Detection
Avoid re-rendering layers that do not change between frames.
*   **Strategy:** `SceneCompiler` marks static layers; `RenderSession` skips them; `FrameCache` stores the framebuffer.
*   **Expected Gain:** 3x-10x for scenes with static backgrounds.

### 4. Precomp Cache
Nested compositions that are static should be cached and reused.
*   **Strategy:** Implement hashing for nested compositions and cache their framebuffers.
*   **Expected Gain:** 2x-8x on complex templates.

### 5. Text Rendering and Layout Cache
Avoid recalculating text layout, word wrapping, and glyph positioning every frame.
*   **Strategy:** Cache layouts using a `TextLayoutKey` (text, font, size, tracking, etc.) and maintain a persistent glyph atlas.
*   **Expected Gain:** 2x-6x for text-heavy videos.

### 6. Effect Cache
Static effects (blur, LUT, vignette) should not be recomputed every frame.
*   **Strategy:** Hash input surfaces and parameters. Use time-varying flags to determine if caching is possible.
*   **Expected Gain:** 2x-5x.

### 7. Resize/Downscale Path
Replace custom C++ scalar resize functions with optimized alternatives.
*   **Strategy:** Use `libswscale` or custom AVX2 SIMD kernels.
*   **Expected Gain:** 2x-4x.

### 8. Surface Allocation and Framebuffer Copies
Reduce memory churn and unnecessary copying.
*   **Strategy:** Use `SurfacePool` aggressively. Move from returning surfaces by value to acquiring/releasing from a pool.
*   **Expected Gain:** 2x-5x and reduced RAM usage.

### 9. PNG and File Intermediates
Eliminate synchronous I/O in the hot path, especially during testing and validation.
*   **Strategy:** Keep AOVs and intermediate surfaces in RAM; avoid per-frame disk writes.
*   **Expected Gain:** 3x-20x in test suites.

### 10. Scene Compilation Caching
Avoid repeated compilation of the same scenes in batch jobs.
*   **Strategy:** Cache `CompiledScene`, `RenderPlan`, and `ExecutionPlan`.
*   **Expected Gain:** 1.5x-4x.

### 11. Asset Decode and Media Manager
Optimize asset loading and decoding.
*   **Strategy:** Implement a real decoder pool and a decoded frame/image cache.
*   **Expected Gain:** 2x-8x.

### 12. 2D Compositor (SIMD)
Optimize the core alpha blending and compositing loops.
*   **Strategy:** Implement AVX2 alpha blending, use premultiplied alpha, and add fast paths for opaque pixels.
*   **Expected Gain:** 4x-15x.

### 13. Masks and Matte
Optimize mask application and generation.
*   **Strategy:** Mask caching, bounding box optimization, and SIMD apply alpha.
*   **Expected Gain:** 2x-6x.

### 14. Motion Blur
Improve efficiency of motion blur sampling.
*   **Strategy:** Use accumulation buffers and adaptive sampling; skip blur for low velocity.
*   **Expected Gain:** 2x-10x.

### 15. Ray Tracer Spatial
Optimize the Spatial backend traversal and sampling.
*   **Strategy:** Use Embree for CPU ray tracing or OptiX for GPU acceleration.
*   **Expected Gain:** 5x-30x.

---

## Implementation Roadmap

### Phase 1: Immediate Performance
1.  Light leak AVX2 implementation.
2.  Remove PNG writes from the hot path.
3.  Integrate `SurfacePool` into the compositor.
4.  Implement AVX2 alpha blending.
5.  Optimize `resize_surface`.

### Phase 2: Architectural Scaling
6.  Static layer detection.
7.  Text layout cache.
8.  Effect cache.
9.  Precomp cache.
10. Frame lookahead parallelism.

### Phase 3: High-Resolution Support
11. Tile rendering in the compositor.
12. Mandatory clip rectangles.
13. Per-job memory budgeting.
14. Intelligent batch worker policy.

### Phase 4: Heavyweight Backend
15. Embree for Spatial ray tracing.
16. Optional compute shaders for heavy effects.

## Zero Waste Philosophy: The Five Pillars of Performance

To reach 0% waste, Tachyon must transition from a "sequence of instructions" mindset to a "hardware-aligned data flow" architecture.

### 1. Memory Flow: SOA (Structure of Arrays) over AOS
The CPU cache is most efficient when reading contiguous memory.
*   **AOS (Wasteful):** `struct Color { r, g, b, a };` forces the CPU to load unnecessary channels.
*   **SOA (Zero Waste):** Separate arrays for `float* r`, `float* g`, `float* b`, and `float* a`.
*   **Action:** Transition `SurfaceRGBA` internal storage to SOA for SIMD kernels. Ensure 32-byte alignment.

### 2. Computational Flow: Branchless SIMD
Every `if` in a pixel loop is a potential pipeline stall.
*   **Strategy:** Use SIMD Masks and `_mm256_blendv_ps` to compute multiple paths in parallel and select the result without jumping.
*   **Action:** Audit effect kernels for internal branching and replace with mask-based logic.

### 3. Algorithmic Flow: Dependency-Based Hashing
The fastest render is the one that never happens.
*   **Strategy:** Implement a rigorous dependency graph where every layer, parameter, and asset generates a unique hash.
*   **Action:** If a layer's state is identical to the previous frame, reuse the cached framebuffer pointer.

### 4. Allocation Flow: Zero-Allocation Render Loop
Memory management is a kernel-level bottleneck.
*   **Strategy:** All intermediate surfaces must be acquired from the `SurfacePool`. No `new`, `malloc`, or `std::vector::resize` during frame execution.
*   **Action:** Enforce `SurfacePool` usage across all effect hosts and compositors.

### 5. Pipeline Flow: Work-Stealing Tiling
Core idle time is the ultimate waste.
*   **Strategy:** Divide frames into tiles (e.g., 128x128). Use a work-stealing scheduler (Taskflow/TBB) to ensure all cores are saturated until the last pixel.
*   **Action:** Replace fixed-thread parallel loops with a tiled task-based architecture.

---

## SIMD Case Study: Light Leak AVX2

Instead of raw Assembly, we use **C++ Intrinsics**. This gives us the power of hand-tuned assembly with the safety and readability of C++.

### Current Scalar "Wastful" Implementation
```cpp
// 1 pixel at a time, lots of branches and expensive calls
Color result = apply_light_leak_style(u, v, t, in, to, style);
```

### Optimized AVX2 "Zero Waste" Implementation
The goal is to process **8 pixels at once** using 256-bit registers.

```cpp
#include <immintrin.h>

void apply_light_leak_avx2(float* dst, const float* src, int count, float t, const Style& style) {
    // 1. Precompute frame-constant values (OUTSIDE the loop)
    __m256 vt = _mm256_set1_ps(t);
    __m256 v_inner = _mm256_set1_ps(style.inner_color.r);
    __m256 v_outer = _mm256_set1_ps(style.outer_color.r);
    // ... load other style params ...

    for (int i = 0; i < count; i += 8) {
        // 2. Load 8 pixels (32 floats: R, G, B, A interleaved)
        // Note: Ideally, use SOA (R8, G8, B8, A8) to avoid shuffles
        __m256 p0 = _mm256_load_ps(&src[i * 4]);     // Pixels 0-1
        __m256 p1 = _mm256_load_ps(&src[i * 4 + 8]); // Pixels 2-3
        // ... load remaining ...

        // 3. Vectorized Math (FMA: Multiply-Add)
        // example: lerp(a, b, t) => a + (b - a) * t
        __m256 diff = _mm256_sub_ps(v_outer, v_inner);
        __m256 mask = _mm256_fmadd_ps(diff, vt, v_inner);

        // 4. Branchless intensity (using masks instead of if)
        // __m256 blend_mask = _mm256_cmp_ps(mask, threshold, _CMP_GT_OQ);
        
        // 5. Store 8 pixels back to memory
        _mm256_store_ps(&dst[i * 4], p0_result);
    }
}
```

### Why this is 15x-30x faster:
*   **Throughput:** 8 pixels per iteration vs 1.
*   **FMA (Fused Multiply-Add):** `(a * b + c)` in a single CPU cycle.
*   **L1 Cache:** Keeping pointers in registers avoids RAM latency.
*   **No Branches:** The CPU pipeline never stalls because there are no `if` checks.

---

## Targeted Symbols and Patterns

Search for and optimize these areas:
`apply_light_leak_style`, `transition_light_leak_template`, `resize_surface`, `save_surface_png`, `render_single_frame`, `RenderSession::render`, `run_render_batch`, `SurfaceRGBA`, `Framebuffer`, `composite`, `blend`, `alpha`, `mask`, `text layout`, `ShapingCache`, `FrameCache`.

## Final Priority Matrix

| Priority | Area | Strategy | Est. Gain |
| :--- | :--- | :--- | :--- |
| 1 | Light Leak Kernel | AVX2/GPU | 10x-30x |
| 2 | PNG/AOV I/O | RAM-only hot path | 3x-20x (tests) |
| 3 | Compositor Alpha Blend | AVX2 + Premultiplied Alpha | 4x-15x |
| 4 | Static Layer Cache | SceneCompiler + FrameCache | 3x-10x |
| 5 | Text Layout | Layout cache + Glyph atlas | 2x-6x |
| 6 | Surface Allocations | SurfacePool integration | 2x-5x |
| 7 | Frame Parallelism | Lookahead queue | 2x-8x |
| 8 | Tile Rendering | Tile-based compositor | 2x-6x |
| 9 | Effects | Effect cache + SIMD | 2x-5x |
| 10 | Resize | libswscale/SIMD | 2x-4x |
| 11 | Asset Decode | Decoded asset cache | 2x-8x |
| 12 | Ray Tracer | Embree/OptiX | 5x-30x |

---

# Spatial Cinematography Engine Vision

Tachyon's ultimate differentiator in the Spatial space is to move away from the manual "layer dragging" paradigm of After Effects and Premiere Pro. Instead, Tachyon should evolve into a **programmable, automatic, verifiable, and generative cinematography engine**, designed for mass video production.

## Core Philosophical Shift
Instead of imperative commands (`place camera here`, `rotate layer X`), Tachyon uses declarative intent:
```cpp
// "I want a dramatic push-in on the title, and ensure it remains readable."
.camera_director()
    .focus("title")
    .style("cinematic_push_in")
    .avoid_occlusion(true)
    .done();
```

## Key Architectural Features

### 1. Auto Camera Director
A programmatic director that automatically calculates camera position, FOV, and motion based on the requested style and subjects. It ensures safe framing and avoids occluding critical elements.

### 2. Visibility Contracts (Occlusion Guard)
Layers have semantic roles and guarantees. A "hero_text" layer can assert `must_be_readable(true)`. The engine validates depth, contrast, and occlusion before rendering.
*   **Validation:** Checks if text is behind a plate, lacks contrast, or is out of frame.
*   **Auto-Fix Engine:** Automatically adjusts Z-depth, boosts emission, adds rim lights, or drops plate opacity if a visibility contract fails.

### 3. Semantic Spatial Layout Solver
Instead of manual X/Y/Z placement, the user provides a list of semantic elements (title, portrait, document) and a layout style (`cinematic_news_wall`). The solver mathematically positions them to avoid overlap and maximize depth separation.

### 4. Depth-Aware Text System
Spatial text is intelligent. It auto-extrudes based on font size, applies safe bevels, auto-emits if background contrast is low, and dynamically corrects its Z-position if occluded.

### 5. Integrated Render Validation
The engine acts as a self-checking system. After a frame is generated, it produces a validation report (`PASS render_not_empty`, `FAIL title_occluded_by_plate`). This is critical for headless, automated workflows.

### 6. Procedural Scene Templates
Parametric data (JSON) drives complex Spatial behaviors rather than static visual presets. Examples include `evidence_wall`, `Spatial_timeline`, or `documentary_reveal`, where the engine handles the choreography of multiple assets dynamically.

## Spatial Implementation Roadmap

*   **Phase 1: Spatial Core Stability** (Accurate camera, depth, materials, lights, Spatial text, debug passes).
*   **Phase 2: Validation Engine** (Automated checks for visibility, contrast, and framing).
*   **Phase 3: Auto-Fix Pipeline** (Algorithmic corrections for failed validations).
*   **Phase 4: Semantic Layout & Auto Director** (Algorithmic layout solvers and programmatic camera movement).
