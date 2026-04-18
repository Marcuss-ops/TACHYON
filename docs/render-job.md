# Render Job

## Purpose

This document clarifies the difference between a reusable scene description and a concrete render request.
That distinction is essential for programmatic rendering workflows.

## Scene spec versus render job

### Scene spec
A scene spec defines the reusable authored blueprint:

- compositions
- layers
- cameras
- animations
- effects
- published properties

### Render job
A render job defines a concrete execution request:

- which scene spec to use
- which template overrides to apply
- which bound data snapshot to inject
- output settings
- seeds or compatibility parameters where required

## Architectural role

The render job is the boundary object between orchestration and the engine core.
The engine should consume render jobs and resolve them into evaluated scenes before raster work begins.

## Design rules

1. Scene specs should remain reusable and stable.
2. Render jobs should carry only the runtime differences needed for one execution.
3. Overrides and data inputs must flow through the template and property systems, not through ad hoc patching.
4. The same render job should be reproducible.

## Guiding principle

A reusable motion template plus a concrete render job should be enough to produce a deterministic output without redefining the entire scene each time.
