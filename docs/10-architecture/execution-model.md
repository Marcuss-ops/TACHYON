---
Status: Canonical
Last reviewed: 2026-05-06
Owner: Core Team
Supersedes: N/A
---

# Execution Model

Tachyon uses a deterministic, frame-based execution model.

## Stages

1. **Scene Parsing** - JSON/C++ spec to scene graph
2. **Resource Loading** - Asynchronous asset pre-fetching
3. **Property Evaluation** - Expression solving and property interpolation
4. **Command Generation** - Generating draw lists for the rasterizer
5. **Rasterization** - Pixel-level rendering
6. **Encoding** - Direct output to FFmpeg pipeline

## Parallelism

The engine supports:
- **Temporal Parallelism**: Multi-threaded frame execution
- **Spatial Parallelism**: Tiled rendering for single high-res frames
- **Pipeline Parallelism**: Concurrent render and encode stages
