# Camera System

## Why cameras are first-class

A camera is not a render option and not an editor trick.
It is a first-class scene system that determines how world-space layers are viewed, projected, and potentially affected by optical properties.

## Camera families

### One-node camera
A one-node camera owns its position and orientation directly.
It is useful for free motion and direct rotational animation.

### Two-node camera
A two-node camera owns a position plus a point of interest.
Its orientation is derived from the target direction.
This allows stable target-tracking motion and familiar motion-graphics camera rigs.

## Point of interest

Point of interest should be treated as a deterministic constraint system, not as a UI-only feature.
At evaluation time, a two-node camera resolves its orientation from:

- camera position
- point of interest target
- optional up-vector rules

## Camera motion operations

The engine should model these as camera or rig operations, not as ad hoc transforms:

- Orbit
- Pan
- Dolly

These operations may later power tools, but the concepts belong in the engine.

## Optical model

The camera system must integrate with optics.
Important parameters include:

- focal length
- field of view
- sensor or gate assumptions
- near clip
- far clip
- focus distance
- aperture
- depth of field parameters

## View generation

Even in a headless engine, the architecture should support multiple view definitions internally:

- active camera perspective
- top orthographic debug view
- side orthographic debug view
- other diagnostic views later

These are useful for future tooling, debugging, and scene inspection.

## Design rules

- The camera system produces transforms, projections, and optical parameters.
- The camera system does not draw pixels.
- Parallax must emerge naturally from projection and world depth.
- Depth of field belongs to optics and downstream compositing, not to random per-layer hacks.
