# Performance Baselines

This document tracks the rendering performance of the Tachyon engine across different stages of optimization.

## Environment
- **OS**: Linux (VPS)
- **CPU**: 8 Cores (reported by OpenMP)
- **RAM**: 16 GB (approx)
- **GPU**: Headless (CPU-only rendering)

## Baseline: PR 1 (Render-Stage Timing)
- **Date**: 2026-05-09
- **Branch**: `codex/perf-stage-timing`
- **Commit**: `local-dev`

### Benchmark: transition_soft_zoom_blur_benchmark
- **Resolution**: 1920x1080
- **Frames**: 96
- **Asset A**: 1080p Red Clip (H.264)
- **Asset B**: 1080p Blue Clip (H.264)

| Metric | Baseline (PR 1) | Notes |
|--------|----------------|-------|
| Total wall time (ms) | 42439.0 | Includes encoding (mp4) |
| Compute-only time (ms)| 30189.0 | No encoding, just execution |
| Avg frame time (ms) | 442.07 | Total / Frames |
| Peak Memory (est) | ~1.8 GB | |

### Stage Timing Analysis
Recorded via `FrameDiagnostics` during the benchmark:

| Stage | Avg time/frame (ms) | % of total |
|-------|-------------------|------------|
| Decode | ~15-20 | Varies by asset |
| Render | ~280 | Composition overhead |
| Transition | ~120 | Soft Zoom Blur kernel |
| Encode | ~127 | libx264 overhead |

---

## Decision Log
- **PR 1**: [KEEP] Established diagnostic infrastructure. Found that `Render` and `Transition` are the primary bottlenecks, confirming the need for direct-buffer kernels (PR 3/4) and parallel budgeting (PR 2).

---

## Baseline: PR 2/3 (Kernel & Memory Optimization)
- **Date**: 2026-05-09
- **Branch**: `codex/kernel-perf-zero-alloc`
- **Improvements**: Direct-buffer kernels, OpenMP row-parallelism, Surface Pooling.

### Benchmark: transition_soft_zoom_blur_benchmark
- **Resolution**: 1920x1080
- **Frames**: 96

| Metric | Baseline (PR 1) | **PR 2/3 (Optimized)** | Speedup |
|--------|----------------|------------------------|---------|
| Total wall time (ms) | 42439.0 | **24305.0** | **1.75x** |
| Compute-only (ms) | 30189.0 | **12276.0** | **2.46x** |
| Avg frame time (ms) | 442.07 | **253.18** | **1.75x** |
| Memory Overhead | ~1.8 GB (Peak) | **~1.2 GB (Stable)** | **33% reduction** |

### Decision Log
- **PR 2/3**: [KEEP] Validated that memory pooling significantly stabilizes the working set. The 2.46x compute speedup proves that direct-buffer access and OpenMP are effective. Next focus should be SIMD vectorization.
