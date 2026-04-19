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

## Canonical shape

A render job should at least define:

- `job_id`
- `scene_ref`
- `composition_target`
- `frame_range`
- `output`
- `overrides`
- `assets_snapshot`
- `seed_policy`
- `compatibility_mode`

## Example

```yaml
job_id: "job_001"
scene_ref: "scene.json"
composition_target: "main"
frame_range:
  start: 0
  end: 120
output:
  format: "mp4"
  path: "out/intro.mp4"
overrides:
  title.text: "Hello"
seed_policy:
  mode: "stable"
compatibility_mode: "v1"
```

## Architectural role

The render job is the boundary object between orchestration and the engine core.
The engine should consume render jobs and resolve them into evaluated scenes before raster work begins.

## Output contract note

The render job should reference explicit output profiles or equivalent structured output settings rather than raw ad hoc encoder flags only.

At minimum, a render job should be able to express:

- target composition
- output destination
- output profile or codec container choice
- frame range or full-range render intent
- audio mode
- quality tier where relevant
- seeds or compatibility locks where relevant

Detailed encoder behavior belongs in `docs/encoder-output.md`.

## Design rules

1. Scene specs should remain reusable and stable.
2. Render jobs should carry only the runtime differences needed for one execution.
3. Overrides and data inputs must flow through the template and property systems, not through ad hoc patching.
4. The same render job should be reproducible.
5. Output-affecting settings must be explicit enough to reproduce the same deliverable.
6. Output targets should be explicit and not inferred from scene authoring alone.

## Guiding principle

A reusable motion template plus a concrete render job should be enough to produce a deterministic output without redefining the entire scene each time.
