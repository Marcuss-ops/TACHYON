# Strategy: AE Quality on Low-End Hardware

This document outlines the architectural strategy for achieving professional-grade output on hardware with limited resources such as low-core CPUs and limited RAM.

## 1. High-fidelity baseline

To get close to the visual credibility people associate with high-end compositing tools, TACHYON should commit to:

- 32-bit floating-point internal compositing where it matters
- explicit linear-light handling for blends, glows, and color-sensitive effects
- deterministic sub-frame accumulation where motion blur is enabled
- strong text, shape, and alpha semantics before chasing broad feature count

The quality target should be stable, believable frames, not accidental sharpness or one-off demo shots.

## 2. Efficiency-first model

To run well on low-end infrastructure, TACHYON should prioritize:

- tile-based or ROI-aware rendering so working sets stay small
- deterministic cache reuse for precomps, passes, and effect outputs
- bounded memory use with explicit quality and fallback policy
- quality tiers that can reduce cost without changing scene semantics
- CPU-friendly data layouts and SIMD-aware hot paths in the 2D renderer

## 3. Hybrid render discipline

Low-end hardware does not mean one renderer should do everything.

TACHYON should use:

- a fast CPU 2D path for text, shapes, masks, mattes, and normal compositing
- a narrow offline CPU 3D path only when a shot truly needs ray-based rendering
- explicit handoff from 3D passes back into the compositor

For the 3D path, the recommended implementation pairing is:

- Intel Embree for acceleration structures and intersections
- Open Image Denoise for controlled low-sample cleanup

## 4. Programmatic rigging and expressions

For data-driven motion, TACHYON should keep the project format declarative and add scripting only where it clearly helps.

Recommended direction:

- declarative scene spec for structure and validation
- Lua as the embedded expression language
- narrow deterministic host APIs
- no dependency on heavy browser or JavaScript runtimes in the render core

Using Lua for expressions is useful.
Using Lua as the entire scene format is not recommended for the core project contract.

## 5. Quality tiers

The engine should expose explicit quality tiers instead of hidden behavior changes:

### Draft
- reduced effect quality where allowed
- proxy media where available
- no expensive motion blur
- low-cost 3D sampling

### High
- normal 2D quality
- approved effect quality
- denoised low-sample 3D where needed
- production output encode settings

### Cinematic
- highest approved 2D quality
- full accumulation blur where enabled
- heavier 3D sample budgets
- richer delivery settings and optional higher bit depth outputs

## 6. What actually matters most on weak machines

The biggest wins usually come from:

- better caching
- tighter invalidation
- smaller working sets
- less unnecessary recompositing
- narrower first-release scope

Low-end success is mostly an architecture problem, not a heroic micro-optimization problem.
