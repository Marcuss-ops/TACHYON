---
Status: Canonical
Last reviewed: 2026-05-06
Owner: Core Team
Supersedes: N/A
---

# Headless Core Architecture

## Overview

Tachyon is designed as a headless engine — it runs without a graphical user interface, making it suitable for server-side rendering, automation pipelines, and batch processing.

## Core Principles

### 1. No GUI, No Display

The engine does not require:
- A windowing system
- A display server (X11, Wayland, etc.)
- GPU-accelerated rendering contexts (unless explicitly using GPU backends)

### 2. Deterministic Execution

Given the same inputs (scene specification, assets, settings), Tachyon must produce byte-identical output.

This requires:
- Deterministic random number generation (seeded properly)
- No reliance on system time or floating-point non-determinism
- Explicit frame scheduling and caching

### 3. CPU-First Design

While Tachyon may optionally use GPU acceleration:
- The primary rendering path is CPU-based
- This ensures portability across server environments
- GPU features are opt-in and clearly separated

## Architecture Layers

```
┌─────────────────────────────────┐
│   C++ Builder API               │  (Programmatic scene authoring)
├─────────────────────────────────┤
│   Scene Specification           │  (JSON or C++-generated)
├─────────────────────────────────┤
│   2D Compositing Engine        │  (Layers, effects, text)
├─────────────────────────────────┤
│   3D Rendering (CPU Path)      │  (Meshes, cameras, lights)
├─────────────────────────────────┤
│   Media I/O Layer              │  (Decode, encode, frame buffers)
├─────────────────────────────────┤
│   Runtime / Execution Engine   │  (Caching, parallelism, scheduling)
└─────────────────────────────────┘
```

## Key Components

### Scene Builder (C++ API)
Type-safe, fluent API for constructing scenes programmatically. See `include/tachyon/scene/builder.h`.

### Render Job
A render job specifies what to render, when, and how. Defined in scene specification.

### Frame Cache
Explicit caching layer to avoid redundant rendering. Configurable memory budgets.

### Output Pipeline
Direct encoding to video formats (via FFmpeg) without intermediate frame dumps.

### Graphics Extension Contract
New graphics features must flow through a neutral contract instead of teaching the renderer to understand authoring data directly. See [Graphics Extension Contract](graphics-extension-contract.md).

## Deterministic Guarantees

1. **Scene parsing** — Same JSON/C++ produces same internal scene graph
2. **Property evaluation** — Expressions evaluate identically across runs
3. **Frame rendering** — Identical frame output for same inputs
4. **Temporal consistency** — Animations, motion blur, and timing are deterministic

## Headless Operation Modes

- **Batch mode** — Render a list of render jobs
- **Service mode** — Long-running process accepting render requests
- **CLI mode** — One-shot command-line rendering
