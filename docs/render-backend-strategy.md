# Render Backend Strategy

## Purpose

This document defines how TACHYON should think about rendering backends without losing its identity as a deterministic, headless motion graphics and compositing engine.

## Core position

TACHYON is not a path tracer first.

TACHYON is a scene engine and compositing engine first. Rendering backends exist to serve scene evaluation, compositing, and encoded output.

That means the project should avoid collapsing all visual work into one monolithic renderer. Different categories of render work may require different execution paths.

## Guiding rule

The engine should separate:

- scene evaluation
- compositing
- 2D raster work
- 3D render work
- encoding

These systems must cooperate through explicit contracts instead of hidden backend coupling.

## Backend classes

### 1. CPU raster backend

This backend is responsible for:

- solids
- images
- text
- vector shapes
- masks and mattes
- most 2D compositing operations
- most image-space effects

This path should remain the default foundation of the engine.

### 2. CPU ray or path tracing backend

This backend is responsible for:

- mesh rendering
- physically based materials
- ray-traced shadows
- reflections and refractions
- global illumination where needed
- depth of field for true 3D scenes
- motion blur for 3D objects and cameras

This path should be treated as a specialized backend for 3D layers, 3D precomps, or render passes.

### 3. Future optional accelerated backends

GPU or hybrid backends may exist later, but they must preserve the same scene model, render contracts, and deterministic semantics where possible.

## Architectural decision

The first serious 3D backend should be CPU-first and deterministic.

This matches the project vision:

- native first
- headless by default
- deterministic first
- CPU-first and low-overhead

## Why this matters

If the project turns into a pure renderer too early, it will lose focus on the harder and more valuable engine problems:

- scene spec quality
- timeline evaluation
- property resolution
- precompositions
- text layout
- effect hosting
- deterministic compositing
- render job overrides

The rendering backend must remain subordinate to the engine contract.

## Proposed first backend split

The first implementation should explicitly distinguish:

- 2D compositing pipeline
- 3D render pipeline
- pass handoff boundary

The 3D backend should render into well-defined outputs that the compositor can consume.

## Render outputs from specialized backends

A backend should be able to produce one or more of the following outputs:

- beauty
- alpha
- depth
- normal
- albedo
- motion vectors
- object id
- material id
- shadow or light contribution passes where relevant

These outputs should be optional and request-driven.

## Recommended direction

Near term, TACHYON should adopt:

- CPU raster for the motion graphics core
- CPU ray or path tracing for serious 3D work
- a compositing contract that can consume both

This allows the project to stay aligned with motion graphics workflows while still gaining physically based 3D capability.

## Non-goal

The project should not attempt to make every layer render through the same physical renderer. Text, vector graphics, masks, and most image processing should remain in dedicated pipelines that are simpler, faster, and easier to control.
