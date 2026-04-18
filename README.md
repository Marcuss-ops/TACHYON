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

## Current status

This repository is intentionally starting from first principles.
The first phase focuses on project foundations and architecture documents only.

See the `docs/` directory for the project vision, architecture, scene model, camera system, compositing model, and roadmap.
