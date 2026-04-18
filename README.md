# TACHYON

TACHYON is a deterministic, headless motion graphics and compositing engine for automated video rendering.

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
- No intermediate frame dumps by default
- Strong scene, camera, compositing, and timeline foundations
- Programmatic, data-driven motion workflows
- Explicit execution, caching, and parallelism models

## Current status

This repository is intentionally starting from first principles.
The first phase focuses on project foundations and architecture documents only.

## Documentation focus

The most important next foundation documents are:

- `docs/scene-spec.md`
- `docs/vertical-slice.md`

These move the project from high-level architecture toward concrete implementation contracts.

## Direction summary

TACHYON is being shaped as a scene engine first, then a compositing engine, then a renderer and encoder.
The goal is not to mimic browser-based video tools, but to build a native temporal dataflow engine for motion graphics and automated rendering.

At its core, the project should evaluate properties in time, derive explicit render work, and produce deterministic output that scales with compute and benefits from caching and parallel execution.
