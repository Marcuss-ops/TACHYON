# Render Strategy

## Purpose

This document defines the rendering strategy TACHYON should commit to early so the rest of the architecture does not drift.

## Core decision

TACHYON should use a hybrid CPU-first rendering strategy:

- a native 2D raster and compositing path for classic motion graphics work
- a native offline 3D rendering path for serious spatial shots
- a shared render graph and compositing model above both

This is the strongest match for the project goals of determinism, headless execution, low-overhead automation, and good results on CPU-only machines.

## Why not one universal renderer

A single rendering path for every task sounds elegant but creates the wrong tradeoffs.

A pure 2D compositor cannot deliver serious 3D lighting, reflections, refractions, global illumination, or physically credible depth of field.
A pure path tracer can deliver excellent 3D imagery, but it is not the right baseline for all motion-design workloads such as text-heavy templates, vector graphics, masks, mattes, adjustment layers, and sharp 2D compositing operations.

TACHYON should therefore avoid forcing all work through one renderer when two specialized paths can remain deterministic and composable.

## Chosen direction for 3D

The preferred offline 3D path should be CPU path tracing built around Embree-class ray intersection infrastructure.

That means the 3D renderer should be designed around:

- Embree for acceleration structures and ray intersection
- a narrow physically based path integrator
- deterministic sampling and seeded randomness
- optional CPU denoising with Open Image Denoise
- explicit AOV support where needed for denoising and compositing

## Why this direction fits TACHYON

This path aligns with the project identity:

- CPU-first
- deterministic
- parallel-friendly
- headless
- suitable for overnight or batch rendering
- capable of materially higher 3D image quality than a fake 2.5D-only path

## 2D path responsibilities

The 2D rendering and compositing path should own the workloads that make a motion engine feel like a serious compositing system:

- solids
- images
- video layers
- text layers
- shape layers
- masks and mattes
- adjustment layers
- blend modes
- effect stacks
- precompositions
- subtitle and template-heavy workflows

This path should prioritize crisp edges, stable sampling, strong alpha behavior, and predictable compositing semantics.

## 3D path responsibilities

The 3D rendering path should own workloads that genuinely benefit from physically based rendering:

- mesh layers
- extruded text
- extruded shapes
- real lights
- materials
- ray-traced shadows
- reflections and refractions
- depth of field
- motion blur
- global illumination
- HDRI image-based lighting

## Shared engine rule

The scene model, property system, animation system, template system, and render graph must remain above the renderer choice.
A camera, keyframed property, expression, template override, or audio-driven control should not care whether the downstream work resolves into the 2D path or the 3D path.

## First implementation guidance

The first shippable implementation should not attempt full feature parity with mature DCC tools.
It should instead prove the architecture with one strong vertical slice.

Recommended order:

1. establish the 2D compositing baseline
2. establish the render graph and pass model
3. add a narrow offline 3D path through Embree-backed path tracing
4. compose 3D outputs back into the 2D stack through explicit passes
5. add denoising and AOV-aware workflows only after the basic pipeline is correct

## Minimum 3D feature slice

The first serious 3D slice should target:

- perspective camera
- camera depth of field with focal length, aperture, focus distance, and bokeh shape
- mesh layer
- extruded text or extruded shape support
- directional, point, spot, area, and environment lighting
- a narrow PBR material model
- beauty, depth, normal, and albedo outputs where useful
- deterministic seeds
- optional denoise stage

## Realistic scope for the first release

The first 3D release should be intentionally narrow. The goal is cinema-grade output for a few important shot classes, not a feature-complete DCC clone.

Recommended initial scope:

- mesh, text extrusion, and shape extrusion
- glTF import for assets that matter
- PBR materials with a compact parameter set
- ray-traced shadows, reflections, refractions, GI, motion blur, and DOF
- IBL through HDRI
- Embree-backed BVH and intersection handling
- OIDN as an optional post-pass for low-sample renders

Not first-release priorities:

- volumes
- cloth
- fluids
- particles
- procedural node-based material systems
- interactive viewport parity
- broad simulation features

## Non-goals for the first 3D slice

The following should not block the first path-traced 3D milestone:

- full volumetrics
- particles
- fluid or cloth simulation
- large material node graphs
- broad procedural geometry catalogs
- interactive preview ambitions

## Guiding principle

TACHYON should be a compositing and motion engine first, with a serious offline 3D renderer where it actually matters.
That is stronger than pretending every shot is either only 2D or only path-traced.
