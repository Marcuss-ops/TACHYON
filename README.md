# TACHYON

TACHYON is a deterministic, headless motion graphics, compositing, and 3D rendering engine for automated and personal video production.

## Core idea

TACHYON is not a browser-based renderer, not a DOM-driven video tool, and not a general-purpose editor.
It is a native engine designed to consume declarative scene specifications and produce encoded video output with minimal overhead.

## Project direction

- C++ first
- Headless and server-side
- Deterministic rendering
- CPU-first architecture
- No browser in the render path
- No JavaScript or TypeScript in the render core
- Hybrid rendering strategy for 2D compositing and offline 3D rendering
- No intermediate frame dumps by default
- Strong scene, camera, compositing, and timeline foundations
- Programmatic, data-driven motion workflows
- Explicit execution, caching, memory, and parallelism models
- Native media decode and encode integration

## Current status

This repository is intentionally starting from first principles.
The current focus is on locking architecture, contracts, and subsystem boundaries before implementation depth grows.

## Local toolchain

If `cmake` or `msbuild` are not visible in a fresh shell, run:

```powershell
.\scripts\Enable-DevTools.ps1 -PersistUserPath
```

This script adds the local CMake and Visual Studio Build Tools locations to the current session and can persist them to the user PATH.

## Build

Use the root build script for the standard build flow:

```powershell
.\build.ps1 -Preset dev -RunTests
```

If you only want to compile, omit `-RunTests`.

For a faster inner loop, use the fast preset and a focused filter:

```powershell
.\build.ps1 -Preset dev-fast -RunTests
.\build.ps1 -Preset dev-fast -RunTests -TestFilter frame_executor
.\build.ps1 -Preset asan -RunTests -TestFilter math
```

`dev` is the normal daily preset. `dev-fast` trims the default build set and applies a focused test filter when you do not pass one yourself. `asan` is for memory and lifetime bugs.

## Documentation

The canonical navigation entry for the documentation set is:

- `docs/README.md`
- `TACHYON_ENGINEERING_RULES.md` — operational rules every contributor should read before changing code

The documentation is organized with a numbered structure so contributors can move from project vision to engine contracts to subsystem behavior in a stable order.

### Documentation layout

- `docs/00-project/` — vision, non-goals, roadmap, MVP scope
- `docs/10-architecture/` — architecture, execution model, determinism, parallelism, scene foundations
- `docs/20-contracts/` — cross-cutting engine contracts such as rendering, surfaces, output, CLI/API, and compatibility
- `docs/30-scene-and-animation/` — scene spec, layers, properties, animation, templates, expressions, and timing
- `docs/40-2d-compositing/` — shapes, masks, effects, text, motion blur, and color-related compositing behavior
- `docs/50-3d/` — cameras, lights, backend strategy, path tracing, and 2D/3D boundaries
- `docs/60-runtime/` — caching, memory policy, quality tiers, profiling, and low-end execution strategy
- `docs/70-media-io/` — asset pipeline, decode/encode, render jobs, and output delivery
- `docs/80-audio/` — audio reactivity and audio-driven animation
- `docs/90-testing/` — canonical scenes and validation-oriented documentation

## Recommended reading order

For a new contributor, the best path is:

1. `docs/README.md`
2. `docs/00-project/vision.md`
3. `docs/00-project/non-goals.md`
4. `docs/10-architecture/architecture.md`
5. `docs/00-project/roadmap.md`
6. `docs/00-project/mvp-v1.md`
7. `TACHYON_ENGINEERING_RULES.md`
8. the relevant contract and subsystem folders for the area being worked on

## Direction summary

TACHYON is a scene engine first, then a compositing and media engine, then a hybrid renderer and encoder.
Specialized render backends may exist, including a CPU-first physically based 3D path, but they remain subordinate to the engine's declarative scene, compositing, and determinism contracts.

At its core, the project should evaluate properties in time, derive explicit render work, choose the appropriate 2D or 3D rendering path, and produce deterministic output that scales with compute and benefits from caching and parallel execution.
