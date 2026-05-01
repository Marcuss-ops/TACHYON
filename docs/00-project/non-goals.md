# Non-Goals

This document explicitly states what Tachyon is NOT trying to be.

## What Tachyon Is Not

- **Not a Video Editor** — No timeline GUI, no interactive editing
- **Not a Browser-Based Tool** — No DOM, no HTML/CSS rendering
- **Not a General-Purpose Game Engine** — Focused on video production, not real-time interaction
- **Not a JavaScript/TypeScript Runtime** — Render core is pure C++, no scripting in render path
- **Not a Real-Time Renderer** — Designed for offline/headless rendering, not real-time playback
- **Not a Frame Dump Tool** — Produces encoded video directly, doesn't require intermediate frames
- **Not a Replacement for DaVinci/After Effects** — Different use case: programmatic, automated, server-side

## Scope Boundaries

Tachyon focuses on:
- Declarative scene specifications (C++ Builder API or JSON)
- Deterministic, high-quality rendering
- Automated video production pipelines

Tachyon explicitly avoids:
- Interactive editing features
- Real-time preview (unless added as separate tool)
- General-purpose 3D gaming features
- Browser-based rendering
