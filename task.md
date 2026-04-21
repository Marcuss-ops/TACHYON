# Task: Milestone 5+ Implementation

## Phase 1: Stabilization & Foundation
- [x] Fix `batch_runner.cpp` build error
    - [x] Implement `build_render_execution_plan` in `render_plan.cpp`
- [x] Finalize Professional Alpha support
    - [x] Refine ProRes 4444 flags in `ffmpeg_pipe_sink.cpp`
    - [x] Update WebM alpha flags
- [x] Implement Adjustment Layers
    - [x] Update `layer_to_draw_command.cpp` to map adjustment layers
    - [x] Implement `Adjustment` command handling in `draw_list_rasterizer.cpp`

## Phase 2: Animation & Expressions
- [x] Implement `valueAtTime(t)` logic
    - [x] Update `expression_engine.cpp` to call back to the host for property sampling
- [x] **PropertyGraph Refactoring**
    - [x] Remove 64-node limit in `property_graph.cpp`
    - [x] Update `PropertyNode` for dynamic dependency tracking
    - [x] Ensure `RenderGraph` handles larger node counts efficiently
- [x] Implement Stagger in `Repeater`
- [x] Implement Motion Path Bezier sampling
- [x] Integrate `OIDN` filter in `frame_executor.cpp`

## Phase 3: Data & Quality
- [x] **System Contracts**
    - [x] Add `version` field to `SceneSpec`
    - [x] Enhance `Diagnostic` with structured metadata
    - [x] Implement deterministic CLI flag management
- [x] Implement Data Connectors
    - [x] Load and parse CSV/JSON files
    - [x] Expose `data()` function to expressions
- [x] Implement Linear Workflow

## Phase 4: Shading & Reflections
- [x] Add single-bounce recursive reflections in `RayTracer`
## Phase 5: Verification (Golden Visual Tests)
- [x] Implement Visual Golden Test framework (PNG/Hash comparison)
- [x] Establish canonical scenes (Simple Transform, Nested Precomp, Expression Stagger)
- [ ] Render glTF test scene
- [ ] Verify emission & bloom-like behavior (Hdr -> ToneMap)
- [ ] Verify denoiser efficiency
- [x] Refactor `tests/unit/test_main.cpp` for Repetition, Seeding, and Listing

## Phase 6: Critical Bug Fixes
- [x] Fix FrameExecutor Caching & Rendering
    - [x] Fix 1: Separate structural and temporal cache keys
    - [x] Fix 2: Implement time-dependent property caching
    - [x] Fix 3: Implement unique keys for Motion Blur samples
    - [x] Fix 4: Add root composition evaluation guarantee
    - [x] Verify with deterministic seeds

## Phase 6 Extended: Engine Hardening & DX
- [x] FFmpeg row-based buffering (reduce syscall overhead)
- [x] Expression engine global variables: `index` and `value`
- [x] After Effects parity for `index * 0.1` stagger patterns
- [x] Unit tests for `index` and `value` in expressions

## Phase 7: Tier 1 Diagnostics (Observability)
- [x] Implement `FrameDiagnostics` struct in `diagnostics.h`
- [x] Update `RenderContext` with diagnostic tracker
- [x] Add manifest support to `CacheKeyBuilder`
- [x] Update `FrameExecutor` to track hits/misses/evaluations
- [x] Verify with 5-step test sequence (Seed/Repeat)

## Phase 8: Hardening & Industrialization
- [ ] Quality Tiers (Draft, Preview, Production)
- [ ] Memory Budget Enforcement (FrameCache LRU)
- [ ] Tiling + ROI (Simplified 4K support)
- [ ] ASAN End-to-End Verification
