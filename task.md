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
    - [x] Implement NativeRenderer utility
- [x] Refactor CLI to support --preset flag
- [x] Implement `tachyon preview` command
- [x] Decouple procedural backgrounds from JSON
- [x] Implement premium text presets (brushed_metal_title)
- [ ] Systematic removal of legacy JSON parser files
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
- [x] Hardening CMake linking (TachyonScene, TachyonRenderer2D, TachyonRuntimeEngine)
- [x] Implement `#ifdef TACHYON_ENABLE_3D` in `scene3d_bridge.cpp` and `composition_renderer.cpp`
- [x] Decouple `TachyonCore` headers (remove heavy runtime/text dependencies from `scene_spec.h`)
- [/] Verify build with `TACHYON_ENABLE_3D=OFF` (In progress, stubs refined)
- [ ] Investigate further modularization (Shapes/Clipper2)

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
- [x] Memory Budget Enforcement (FrameCache LRU)
- [x] Tiling + ROI (Smart Invalidation)
- [ ] ASAN End-to-End Verification

## Phase 9: Production Pipeline & Subsystems
- [x] Variable-feather Bezier masks (implementation + eval)
- [x] AI-assisted RotoBrush engine (FeatureTracker integration)
- [x] Multi-channel Audio Mixer (panning, limiter, sinc-interpolation)
- [x] AudioVideoExporter (frame-accurate sync)
- [x] Persistent Disk Cache system
- [x] Playback Engine (background rendering, pre-fetching, ROI)
- [x] Physical Camera & 3D Solver (PnP DLT, Motion Blur, DoF, Shake)
- [x] Modular roadmap documentation (tracking, roto, playback, etc.)
- [x] Vectorized Mask Rasterizer (AVX2)
- [x] Multi-Camera Timeline Cuts (Composition explicit cuts)

## Phase 10: Severe Cleanup (Architectural Consolidation)
- [x] Sincronizzazione branch `feat/layer3d-mesh-resolver` con `main`
- [x] Integrazione test `scene3d_smoke_tests.cpp` in CMake
- [x] Refactor JSON Parsing (PR #1: Primitive vs Domain)
    - [x] Creazione `json_reader.h/cpp` per primitive
    - [x] Creazione `spec_value_parsers.h/cpp` per dominio
    - [x] Migrazione core math (Vector, Color)
    - [x] Pulizia call sites in `json_parse_helpers.cpp`
- [x] Refactor Animation Parsing (PR #2: Easing/Bezier)
    - [x] Creazione `easing_parse.h/cpp`
    - [x] Migrazione logica easing e bezier
    - [x] Conversione `json_parse_helpers.cpp` in adapter legacy
- [/] Refactor Effect Utilities (PR #3: Specialized Modules)
    - [x] Split `effect_utils.cpp` in `color_math`, `surface_sampling`, etc.
    - [x] Creazione nuovi header in `renderer2d/`
    - [x] Registrazione nuovi file in CMake
    - [/] Validazione build e test
- [ ] Refactor Binary Serialization (PR #4: TBF Helpers)
    - [ ] Audit `tbf_read_helpers.cpp`
    - [ ] Creazione test di roundtrip
- [ ] Enforcing Module Boundaries
    - [ ] Audit degli header pubblici
    - [ ] Rimozione inclusioni circolari residue
