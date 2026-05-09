# Transitions

This document describes how transitions work today and how to add new ones without changing the existing behavior model.

## Current State

- `TransitionPresetRegistry` is intentionally empty by default.
- The transition builder functions are thin pass-through helpers.
- `build_transition_enter()` and `build_transition_exit()` only map the caller request into a `LayerTransitionSpec`.
- Empty or `none` ids mean no transition.
- Unknown ids are preserved as explicit authoring input, but they now resolve to a no-op transition instead of a silent visual fallback.

## Data Flow

1. A caller fills `TransitionParams`.
2. The builder forwards the requested `id` to the registry.
3. The registry returns either a matching spec or a no-op spec for unknown ids.
4. The layer stores the resulting `LayerTransitionSpec`.
5. The runtime consumes the transition exactly as authored.

## Extension Points

- `include/tachyon/presets/transition/transition_params.h`
- `include/tachyon/presets/transition/transition_builders.h`
- `src/presets/transition/transition_builders.cpp`
- `src/presets/transition/transition_presets_table.cpp`
- `src/core/transition_registry.cpp`
- `src/renderer2d/effects/core/glsl_transition_effect.cpp`

## How to Add a Transition

1. Add a transition implementation in the runtime path when the transition needs a pixel effect.
2. Register the runtime transition id in `src/core/transition_registry.cpp` if the engine needs to look it up by id.
3. Add a preset alias in `src/presets/transition/transition_presets_table.cpp` only if you want an authoring shortcut.
4. Use the explicit transition id from scene code or layer builders.
5. Keep the builder path thin.

## Rules

- Do not restore a hidden default transition.
- Do not silently map unknown ids to a visual fallback.
- Do not duplicate transition kernels in multiple registries.
- Do not change the scene authoring contract just to add a new transition name.

## Practical Result

- `none` stays `none`.
- An explicit transition id stays explicit.
- Runtime effects and authoring aliases stay separate concerns.

## Performance Optimizations (Changelog 2026-05-09)

### Done Today
- **Direct-Buffer Kernels**: Removed `set_pixel()` overhead from the hot path. Transition kernels now write directly to raw pixel buffers with pointer arithmetic.
- **OpenMP Parallelization**: Transitions are now parallelized at the pixel-row level using OpenMP, respecting the hardware thread budget.
- **Bilinear Fast-Path**: Optimized texture sampling with a fast-path for integer-aligned coordinates, avoiding interpolation math when unnecessary.
- **Same-Size Fast-Path**: Added a specialized kernel for transitions where source and destination surfaces have identical dimensions, skipping UV transformations.
- **Zero-Allocation Rendering**: Integrated `renderer2d::SurfacePool` into the composition pipeline. Temporary transition buffers are now recycled from a pool instead of being allocated/deallocated per frame.

### Benchmark Results
*Benchmark: `soft_zoom_blur` @ 1080p, 96 frames, 8-core CPU*

| Metric | Baseline (PR 1) | **Optimized (PR 2/3)** | Speedup |
|--------|----------------|-----------------------|---------|
| Total wall time (ms) | 42,439.0 | **24,305.0** | **1.75x** |
| Compute-only (ms) | 30,189.0 | **12,276.0** | **2.46x** |
| Memory Overhead | Raw Allocations | **Pooled Reuse** | **Near-Zero** |

### Next Goals
- [ ] **SIMD Kernels**: Implement AVX2/SSE4.2 vectorized kernels for standard blend modes and transitions.
- [ ] **Pre-multiplied Alpha**: Shift the entire 2D pipeline to pre-multiplied alpha to avoid redundant alpha divisions in the hot path.
- [ ] **Cache Locality**: Optimize tile rendering order to improve CPU L3 cache hit rates during heavy composition.
