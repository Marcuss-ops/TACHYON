# Strategy: AE Quality on Low-End Hardware

This document outlines the architectural strategy for achieving professional-grade cinematic output on hardware with limited resources (low-core CPUs, limited RAM).

## 1. High-Fidelity Rendering Core

To achieve the "After Effects Look", TACHYON commits to:

- **32-Bit Linear Workflow**: All internal compositing and effect calculations occur in 32-bit floating-point linear light space. This prevents "clipping" in glows and ensures physically accurate color blending.
- **Accumulated Motion Blur**: Instead of simple post-process blurs, we use sub-frame accumulation. Deterministic sampling ensures that even with low samples, the grain is stable and identifiable.
- **SIMD Rasterization**: A native C++ vector engine hand-tuned for SSE/AVX (and NEON on ARM) ensures that sub-pixel anti-aliasing is performant enough for complex shapes without requiring a GPU.

## 2. Low-End Optimization (The "Efficiency First" Model)

To run on "computer di fascia bassa":

- **Tile-Based Rendering (ROI)**: frames are divided into tiles (e.g., 256x256). This allows the engine to keep the working pixel set within the CPU's L3 cache, drastically reducing memory bandwidth bottlenecks.
- **Zero-Copy Media Pipeline**: Direct integration with hardware-accelerated decoders where available, or highly optimized SIMD YUV->RGBA transforms to minimize CPU overhead during video playback.
- **Incremental Cache**: Deterministic caching of sub-graphs ensures that only modified parts of a composition are re-rendered.

## 3. Programmatic & Data-Driven Rigging

- **Lua JIT Expression Engine**: Replacing heavy JS runtimes with Lua JIT for low-latency evaluation of complex rigs and data-driven animations.
- **Constraint-Based Layout**: Unlike AE's fixed coordinates, TACHYON uses a layout system tailored for dynamic content, allowing "responsive" programmatic video.

## 4. Quality vs. Speed Tiers

The engine supports explicit quality tiers:
- **Draft**: Low samples, no motion blur, proxy assets.
- **High**: Full samples, OIDN (Open Image Denoise) on CPU, high-bitrate output.
- **Cinematic**: Path-traced 3D elements, full accumulation blur, 4:4:4 output.
