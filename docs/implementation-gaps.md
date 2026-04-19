# Implementation Gaps

## Purpose

This document summarizes what still needs to exist before TACHYON can feel like a headless After Effects for programmatic video production.

## 1. Runtime of the scene graph

Missing core systems:

- parser and validator for the scene spec
- unified animatable property model
- deterministic per-frame evaluator
- dependency graph and cache invalidation
- parallel scheduler for frame and pass work

## 2. Serious 2D compositing

Missing core systems:

- CPU vector rasterizer
- complete shape layer support
- real text layout and shaping
- masks, mattes, and blend modes
- precomps and adjustment layers
- deterministic effect graph
- correct alpha handling

## 3. Media pipeline

Missing core systems:

- image, video, and audio decode
- output encode
- color management
- linear and gamma workflow
- media proxy and cache support
- subtitle pipeline
- audio analysis and reactivity

## 4. CPU offline 3D

Missing core systems:

- Embree BVH and ray intersection
- compact integrator
- Open Image Denoise integration
- compact PBR materials
- camera, lights, DOF, and motion blur policy
- mesh ingest
- later text and shape extrusion

## 5. Programmatic interface

Missing core systems:

- clean CLI
- stable API surface
- versioned scene spec
- template overrides
- Lua sandbox for expressions
- declarative render jobs

## 6. Performance on weak machines

Missing core systems:

- tiling and ROI
- progressive sampling where appropriate
- quality tiers
- memory budgets
- frame and pass caching
- resume after crash
- chunked export

## 7. Decisions that should be locked before heavy implementation

Before writing large amounts of engine code, TACHYON should explicitly lock these contracts:

- coordinate system and handedness
- units for distance, focal length, and timing
- internal alpha model
- internal color precision and working space
- layer and precomp time semantics
- random seed policy for deterministic noise and sampling
- render surface and AOV naming conventions
- memory budget behavior under pressure
- error and diagnostics model
- compatibility versioning rules for visible output changes

## 8. Test assets that should exist early

The project should also create a small permanent validation pack:

- one text-heavy scene
- one shape-heavy scene
- one masks and mattes scene
- one subtitle or word-timing scene
- one low-sample 3D scene
- one caching and invalidation scene

These should become golden scenes early so performance work does not silently change output.

## Recommended order

1. lock the core contracts in section 7
2. runtime of the scene graph
3. serious 2D compositing
4. media pipeline
5. programmatic interface
6. CPU offline 3D
7. performance and scale

## Guiding rule

If a feature does not improve deterministic output, automation, or render quality, it should wait.
