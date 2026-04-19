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
The first phase focuses on project foundations and architecture documents before implementation depth arrives.

## Documentation map

Core project documents:

- `docs/vision.md`
- `docs/non-goals.md`
- `docs/architecture.md`
- `docs/roadmap.md`
- `docs/mvp-v1.md`
- `docs/render-strategy.md`
- `docs/pixel-pipeline.md`

Engine model documents:

- `docs/execution-model.md`
- `docs/property-model.md`
- `docs/determinism.md`
- `docs/parallelism.md`
- `docs/render-job.md`
- `docs/scene-spec.md`
- `docs/composition-layer-model.md`
- `docs/camera-and-3d-scene.md`
- `docs/dependency-graph-and-invalidation.md`

System design documents:

- `docs/expression-system.md`
- `docs/effects.md`
- `docs/masking.md`
- `docs/shape-system.md`
- `docs/implementation-gaps.md`
- `docs/3d-mesh-and-extrusion.md`
- `docs/text-animator.md`
- `docs/text-layout-and-shaping.md`
- `docs/light-system.md`
- `docs/lighting-and-materials.md`
- `docs/time-system.md`
- `docs/motion-blur.md`
- `docs/template-system.md`
- `docs/color-management.md`
- `docs/caching.md`
- `docs/audio-reactivity.md`
- `docs/data-binding.md`
- `docs/rendering-contract.md`
- `docs/asset-pipeline.md`
- `docs/decode-encode.md`
- `docs/voice-over-and-subtitles.md`
- `docs/multi-format-output.md`
- `docs/testing-and-compatibility.md`

## Direction summary

TACHYON is being shaped as a scene engine first, then a compositing and media engine, then a hybrid renderer and encoder.
The goal is not to mimic browser-based video tools, but to build a native temporal dataflow engine for motion graphics, automated rendering, serious text and subtitle workflows, and offline 3D image synthesis.

At its core, the project should evaluate properties in time, derive explicit render work, choose the appropriate 2D or 3D rendering path, and produce deterministic output that scales with compute and benefits from caching and parallel execution.
