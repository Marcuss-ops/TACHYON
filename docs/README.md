# TACHYON Documentation

Welcome to the documentation for **TACHYON**, a high-performance, deterministic, headless 2D motion graphics and compositing engine built for automated and personal video production.

---

> [!NOTE]
> TACHYON is a native C++ engine designed to consume declarative scene specifications and produce encoded video output with minimal overhead. It is not browser-based, has no DOM, and operates entirely in server-side environments.

---

## Subsystem Navigation

Our documentation is structured around the concrete, functional systems implemented in the codebase:

- **[Headless Architecture](architecture.md)** — Core architecture layers (Compositor, Runtime, Media I/O) and deterministic execution guarantees.
- **[Public C++ API Contract](api.md)** — Fluent, Remotion-style authoring API including scene/composition builders, layer styling, and preset animators.
- **[Parallel Rendering Pipeline](parallelism.md)** — Technical deep dive into Tachyon's multi-tiered parallel execution models (Batch, Frame, Tile, and Layer scheduling).
- **[CLI & Rendering Workflow](rendering.md)** — Step-by-step instructions for compiling targets and executing declarative renders from the command line.

---

## Architectural Truths

Every contribution to TACHYON must adhere to these foundational principles:

### 1. Zero Waste Performance
- **Structure of Arrays (SOA)** memory layouts for pixel-intensive buffers.
- **AVX2 / SIMD Intrinsic kernels** for all hot-path rasterization and blending.
- **Zero dynamic memory allocations** in the active render loop (facilitated by a pre-allocated `SurfacePool`).

### 2. High-Performance 2D Compositing
- Pure C++ compositing logic bypassing DOM overhead.
- Multi-threaded rendering utilizing tiled rasterization and work-stealing schedulers.
- Procedural elements, layer blending, alphas, masking, and audio reactivity.

### 3. Programmatic & Deterministic Authoring
- Fully type-safe builder interface to guarantee valid scene generation at compile time.
- Byte-identical rendering outputs for identical inputs, enabling perfect visual regression tracking.
