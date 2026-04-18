# Vision

## What TACHYON is

TACHYON is a native, deterministic, headless motion graphics and compositing engine.
Its purpose is to transform declarative scene specifications into encoded video output as efficiently and predictably as possible.

The engine is designed for serious rendering pipelines, automated media generation, server-side video production, and future high-performance motion workflows that require strong control over scene evaluation, camera systems, compositing, and output determinism.

## Core principles

### 1. Native first
The render path must remain native C++.
No browser, no DOM, no Chromium, no JavaScript runtime in the render core.

### 2. Determinism first
Given the same spec, assets, and engine version, the output should be reproducible.
Determinism is a product feature, not a side effect.

### 3. Headless by default
TACHYON is not being built as a GUI editor.
Any future tooling must remain secondary to the core headless engine.

### 4. Scene-driven architecture
The project must be organized around scenes, layers, transforms, cameras, compositions, render passes, and encoded output.
Not around UI metaphors or browser abstractions.

### 5. CPU-first and low-overhead
The initial design must work extremely well on CPU-only infrastructure.
GPU acceleration may exist later, but the core architecture must not depend on it.

### 6. Compositing matters
TACHYON is not just a frame blitter.
It should grow into a true motion graphics and compositing engine with support for hierarchy, precomps, cameras, 2D and 3D layers, and multi-pass rendering.

### 7. Data in, video out
The engine consumes structured scene descriptions and assets.
It does not own orchestration, business logic, content generation, or publishing.

## Product shape

Long term, TACHYON should feel closer to a native scene and compositing engine than to a web-based video framework.
It should support:

- declarative compositions
- timed layers
- 2D and 3D transforms
- camera systems
- parenting and rigging
- precompositions
- compositing passes
- deterministic rendering
- native encoding

## First milestone philosophy

The first milestone is not about being flashy.
It is about creating a clean architectural foundation that can grow without becoming a tangled renderer.

That means writing down:

- what the engine is
- what it is not
- which systems are first-class
- which tradeoffs are intentionally accepted
