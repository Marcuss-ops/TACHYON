# Surface and AOV Contract

## Purpose

This document defines the named render surfaces and auxiliary outputs that TACHYON should use internally and expose where appropriate.

A strong naming and semantics contract here is important for compositing, denoising, debugging, caching, and output reproducibility.

## Core rule

A surface name should imply stable semantics.

The engine should not use the same name for outputs with materially different meaning across backends or quality modes.

## Why this matters

Without a contract, these problems appear quickly:
- denoisers receive mismatched inputs
- compositing code depends on backend quirks
- cache keys become ambiguous
- debug output is inconsistent
- 2D and 3D handoff becomes fragile

## Surface classes

Recommended initial classes:

### Primary image surfaces
Core visual outputs intended for final composition.

### Auxiliary surfaces or AOVs
Data outputs used for denoise, debugging, analysis, or later effects.

### Mask and matte surfaces
Alpha-like or grayscale routing surfaces used in compositing logic.

## Minimum surface set

The first implementation should define at least:

### `beauty`
The main rendered color result intended for final composition.

### `alpha`
The explicit alpha result associated with the primary image output where applicable.

### `depth`
A depth-related auxiliary output for 3D workflows.
Its metric and direction conventions must be documented.

### `normal`
A normal-space auxiliary output for 3D workflows.
The space convention must be fixed.

### `albedo`
A base-color style auxiliary output where useful for denoising or debug.

## Optional early surfaces

These should remain optional until needed:
- motion later if a motion-vector workflow is introduced
- object_id later if segmentation becomes useful
- material_id later if debug workflows need it
- emissive later if the 3D renderer grows enough to justify it

## Surface semantics

Each named surface should define:
- data meaning
- coordinate or space convention where relevant
- precision expectation
- alpha expectations if relevant
- whether it is compositing-facing, denoise-facing, or debug-facing

## Beauty contract

`beauty` should mean:
- the primary visible image result for that pass
- color semantics compatible with the current render and compositing contract
- no hidden ambiguity about whether it is before or after alpha association

## Depth contract

The engine should choose one explicit first definition.

Recommended first rule:
- depth stores camera-relative distance information
- the direction, normalization, and nonlinearity policy must be documented explicitly
- the same backend should not emit different depth conventions under the same name

## Normal contract

Recommended first rule:
- normal uses a clearly declared space such as world space or camera space
- the first implementation should pick one and hold it steady
- channel interpretation must be documented

## Alpha contract

Alpha should never be guessed from color outputs.
If a pass supports alpha, it should be explicit.
If a pass does not support alpha meaningfully, that should also be explicit.

## 2D and 3D handoff rule

The 3D path should hand surfaces back to the compositor through named outputs, not through backend-specific assumptions.

That means:
- 3D render passes emit explicit surfaces
- compositing code consumes those surfaces by contract
- denoise inputs use the same explicit names

## Mask and matte surfaces

Mask and matte routing should also use stable names where intermediate surfaces are materialized.
Useful first concepts include:
- mask_alpha
- matte_alpha
- luma_matte later if explicitly materialized as a surface

The exact initial internal names may remain implementation-defined, but the semantic categories should be documented.

## Caching implications

Surface identity must be part of cache identity.

A cached `beauty` output is not the same thing as a cached `depth` output.
Likewise, a `beauty` surface produced by a 2D pass is not semantically interchangeable with a `beauty` surface from a different scoped pass unless the contract says it is.

## Output and debug policy

The runtime should be able to expose surfaces for:
- final compositing
- debug dumps when explicitly requested
- denoise inputs where relevant

Surfaces should not be dumped by default just because they exist internally.

## First implementation recommendation

The first stable named surface set should be:
- beauty
- alpha
- depth
- normal
- albedo

Anything beyond that should be added deliberately once a real consumer exists.

## Guiding principle

TACHYON should use a small stable surface vocabulary first.
That makes compositing cleaner, denoise workflows safer, and debugging more reliable as the engine grows.
