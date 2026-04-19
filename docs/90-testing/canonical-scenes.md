# Canonical Scenes

## Purpose

This document defines the permanent validation scene pack that TACHYON should keep alongside its renderer and contracts.

These scenes are not marketing demos.
They are regression anchors.

## Core rule

Canonical scenes should exist to validate engine behavior, not to look impressive.

Each scene should test a small number of important contracts clearly.
The project should prefer a compact stable pack over a huge uncurated gallery.

## Why this matters

Canonical scenes support:
- visual regression testing
- compatibility review
- cache and invalidation checks
- performance comparison
- profiling repeatability
- onboarding for contributors

## Required metadata for each scene

Each canonical scene should declare:
- scene id
- short description
- feature focus
- expected outputs
- whether it is 2D, 3D, or hybrid
- whether it has golden frames or golden summaries

## Minimum first pack

The first permanent pack should include at least the following scenes.

### 1. Text-heavy scene
Focus:
- font loading
- shaping
- line layout
- per-character animation
- subtitle-like timing or dense text motion

### 2. Shape-heavy scene
Focus:
- vector paths
- strokes and fills
- trim paths
- repeater behavior
- stable antialiasing and rasterization

### 3. Masks and mattes scene
Focus:
- mask stack behavior
- feather and expansion
- alpha and luma matte behavior
- adjustment-style compositing semantics where available

### 4. Subtitle or word-timing scene
Focus:
- timed text
- word-level or segment-level animation
- audio-aligned or externally timed overlays where applicable

### 5. Low-sample 3D scene
Focus:
- Embree-backed or equivalent 3D path basics
- beauty, depth, normal, and albedo surfaces where available
- denoise-adjacent validation later
- deterministic 3D output under fixed seeds

### 6. Caching and invalidation scene
Focus:
- selective scene changes
- precomp reuse
- effect caching where available
- invalidation correctness versus over-invalidation

## Optional early additions

Useful next scenes may include:
- motion blur scene
- output and encode stress scene
- large precomp nesting scene
- mixed 2D and 3D compositing boundary scene

## Golden output policy

Canonical scenes should support golden validation through one or more of:
- golden frames
- golden surface dumps when explicitly useful
- golden structured summaries later where appropriate

Not every scene needs every possible artifact.
Each scene should keep only the artifacts required to validate its intended contract.

## Performance policy

Some canonical scenes should also have stable profiling value.

That means:
- fixed seeds
- fixed quality tier
- fixed output profile where relevant
- explicit expected workload class

This helps compare performance changes across revisions.

## Compatibility use

When visible-output behavior changes intentionally, canonical scenes should help answer:
- what changed
- why it changed
- whether the change is acceptable
- whether compatibility notes need to be updated

## Folder strategy

The scene pack should eventually live in a stable location with:
- scene definitions
- referenced assets
- expected outputs or manifests
- short per-scene readme notes where needed

## First implementation recommendation

The project should create the first six scenes before claiming renderer stability.

If a new subsystem is hard to test against the canonical pack, that is often a sign that its contract is not yet clear enough.

## Guiding principle

Canonical scenes are the engine's memory.
They preserve what TACHYON means when it says deterministic, programmable, and visually stable.
