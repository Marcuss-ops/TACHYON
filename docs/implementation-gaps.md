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
- compact path tracer
- Open Image Denoise integration
- glTF import
- compact PBR materials
- camera, lights, DOF, and motion blur
- mesh and text extrusion

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
- progressive sampling
- quality tiers
- memory budgets
- frame caching
- resume after crash
- chunked export

## Recommended order

1. runtime of the scene graph
2. serious 2D compositing
3. media pipeline
4. programmatic interface
5. CPU offline 3D
6. performance and scale

## Guiding rule

If a feature does not improve deterministic output, automation, or render quality, it should wait.
