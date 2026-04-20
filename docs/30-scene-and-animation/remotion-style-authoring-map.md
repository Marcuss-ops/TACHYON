# Remotion-Style Authoring Map

## Goal

This document maps the small, pure, data-driven authoring model we want for TACHYON onto the current C++ engine.

The target is not a web framework.
The target is a native, deterministic motion runtime where:

- time is an input
- animation is a pure function of frame/time
- reusable components are C++ factories
- data is explicit and versioned
- the render loop does not parse or interpret authoring structures

## Surface we want

Keep the public authoring surface small:

- `Composition`
- `Sequence`
- `Layer`
- `animate(...)`
- `interpolate(...)`
- `spring(...)`
- `compose(...)`
- `load_data(...)`
- `render(...)`

If a feature does not reduce mental load, it does not belong in the first pass.

## Core principle

Every animation evaluator must behave like:

```cpp
value = eval(track, t);
```

Same input, same output.
No hidden clock.
No implicit global timeline.
No side effects inside evaluation.

## Implementation plan

### Phase 1. Define the compiled runtime boundary

Create a compiled scene representation that is distinct from authored scene data.

Deliverables:

- `CompiledSceneHeader`
- `CompiledScene`
- stable versioning and layout versioning
- explicit determinism flags

Implementation targets:

- `include/tachyon/runtime/compiled_scene.h`
- `src/runtime/scene_compiler.cpp`

Acceptance:

- scene data can be compiled once and consumed by the runtime
- compiled artifacts can be invalidated when layout or determinism settings change

### Phase 2. Build the minimal pure-time API

Implement the small math/animation library used by the authoring layer.

Deliverables:

- `interpolate(frame, in, out, policy)`
- `spring(config)`
- easing helpers
- clamp, lerp, mapRange, noise

Implementation targets:

- `include/tachyon/core/animation/`
- `include/tachyon/runtime/expression_vm.h`

Acceptance:

- the same frame produces the same value across runs
- interpolation and spring behavior are deterministic
- no UI-specific logic leaks into the runtime

### Phase 3. Introduce component factories

Components should be reusable C++ factories, not copy-pasted scene blobs.

Deliverables:

- `CompositionFragment`
- `LayerGroup`
- factory functions that take props and return reusable scene fragments

Example shape:

```cpp
LayerGroup make_title(const TitleProps& props);
```

Implementation targets:

- `include/tachyon/core/spec/`
- `src/core/spec/scene_spec_builder.cpp`

Acceptance:

- one component can be reused many times with different props
- composition nesting is explicit and deterministic

### Phase 4. Make data explicit

The authored scene must be separable from the data snapshot used for rendering.

Deliverables:

- `DataSnapshot`
- explicit binding of scene inputs to data fields
- compile-time resolution of supported data references

Implementation targets:

- `include/tachyon/runtime/render_job.h`
- `src/runtime/render_job.cpp`
- `src/core/spec/scene_spec_json.cpp`

Acceptance:

- changing data changes output without rewriting scene structure
- render jobs remain reproducible from explicit inputs

### Phase 5. Add deterministic cache keys

Cache keys must be derived from explicit compiled inputs.

Deliverables:

- `CacheKeyBuilder`
- `FrameCacheKey`
- cache invalidation by dependency mask

Inputs to hash:

- compiled scene hash
- frame number
- global seed
- dependency mask
- determinism policy
- layout version

Implementation targets:

- `include/tachyon/runtime/cache_key_builder.h`
- `src/runtime/frame_cache.cpp`
- `src/runtime/render_graph.cpp`

Acceptance:

- cache hits are stable across runs
- invalidation is selective and deterministic

### Phase 6. Add a frame arena

All transient per-frame allocations should come from a resettable arena.

Deliverables:

- `FrameArena`
- `std::pmr::monotonic_buffer_resource` backed by a fixed buffer
- zero-free reset at frame start

Implementation targets:

- `include/tachyon/runtime/frame_arena.h`
- `src/runtime/frame_executor.cpp`

Acceptance:

- frame execution does not allocate from the general heap in the hot path
- temporary evaluation data is reclaimed by reset, not by free

### Phase 7. Compile expressions to bytecode

Expressions should be compiled, not interpreted from strings during frame execution.

Deliverables:

- `ExpressionCompiler`
- `ExpressionVM`
- compact bytecode instruction format
- deterministic builtin operations

Suggested opcode set:

- `PushConst`
- `LoadTime`
- `LoadFrame`
- `LoadProp`
- `Add`
- `Sub`
- `Mul`
- `Div`
- `Sin`
- `Cos`
- `Clamp`
- `Lerp`
- `Noise`
- `Spring`
- `Return`

Implementation targets:

- `include/tachyon/runtime/expression_vm.h`
- `src/core/expressions/expression_engine.cpp`

Acceptance:

- expressions evaluate without AST traversal in the frame loop
- no host callbacks are required for common motion primitives

### Phase 8. Add a property dependency graph

Dependencies should be represented explicitly so invalidation is cheap.

Deliverables:

- `PropertyGraph`
- node bitmasks for dependencies and invalidation
- stable topological ordering

Implementation targets:

- `include/tachyon/runtime/property_graph.h`
- `src/runtime/render_graph.cpp`

Acceptance:

- changing one property invalidates only dependent work
- the traversal order is stable for the same scene and policy

### Phase 9. Move frame execution to the compiled path

The runtime should consume compiled data directly.

Deliverables:

- compiled frame evaluator
- compiled render task execution
- profiling hooks

Implementation targets:

- `src/runtime/frame_executor.cpp`
- `include/tachyon/runtime/frame_executor.h`

Acceptance:

- `SceneSpec` is not walked repeatedly in the hot path
- the runtime operates on compact compiled data

### Phase 10. Lock determinism with tests

Golden tests are the guardrail for every optimization.

Deliverables:

- frame hashes for selected frames
- cross-build regression checks
- render output comparisons

Implementation targets:

- `tests/unit/runtime/`
- `tests/integration/`

Acceptance:

- frames 0, 15, and 147 match reference hashes
- SIMD or layout changes cannot silently change output

## Recommended order

1. compiled scene boundary
2. pure-time helper API
3. component factories
4. explicit data inputs
5. cache keys and invalidation
6. frame arena
7. expression bytecode
8. dependency graph
9. compiled frame execution
10. golden tests

## Non-goals for the first pass

- full editor UI
- plugin ecosystem
- arbitrary scripting in the render path
- host callbacks inside the frame loop
- JSON as the primary authoring surface

## Success criterion

The implementation is good when a user can:

- define reusable components
- pass data explicitly
- evaluate at a frame or time input
- render deterministically
- debug a frame without guessing hidden state

