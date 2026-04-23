# Scene Contracts & Serialization

> **Purpose:** define the shared contracts that all next-step features rely on.

## Why this exists

The split plans introduce a lot of new references and dependency edges:
- track bindings
- planar tracks
- camera tracks and cuts
- matte sources and targets
- time remap and blend modes
- proxy variants and cache identities
- audio tracks and export settings

If each feature invents its own schema, the editor, runtime, and import/export paths will drift apart.
This document is the common layer that the feature plans should reference.

## What the contract should cover

### Identity and references
- Stable IDs for layers, tracks, cameras, audio tracks, and matte sources
- Explicit reference fields instead of positional assumptions
- Validation for dangling references and duplicate IDs

### Time model
- Composition time
- Source media time
- Shutter interval time for temporal effects
- Frame index vs time in seconds, with one canonical conversion path

### Versioning
- Schema version on every persisted scene
- Forward-compatible defaults for missing fields
- Migration path for renamed fields and moved features

### Dependency edges
- Track binding edges
- Matte source/target edges
- Camera cut edges
- Proxy/original media mapping
- Cache invalidation dependencies

## Required shared types

These types should be defined once and reused by the feature plans:
- `TrackBinding`
- `PlanarTrack`
- `CameraTrack`
- `CameraCut`
- `MatteDependency`
- `TimeRemapCurve`
- `FrameBlendMode`
- `ProxyVariant`
- `AudioTrackSpec`

## Validation rules

- No dangling references
- No matte cycles
- No overlapping hard camera cuts
- No duplicate track or layer IDs
- Cache keys must include every field that changes visible output

## Recommended order

1. ✅ Define the shared scene schema and versioning strategy
2. ✅ Add or update feature-specific contracts on top of that schema
3. ⬜ Wire evaluator, renderer, and editor to the shared references
4. ⬜ Add migration tests before feature expansion

