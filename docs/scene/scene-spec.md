# Canonical Scene Spec v0

## Purpose

This document defines the first concrete external scene contract for TACHYON.
It turns architectural direction into a buildable input model.

This is the authoritative spec contract for authored scene data. The `scene-spec-v1.md` document remains a companion summary, but this file carries the formal shape.

## Top-level shape

A scene document should contain:

- `spec_version`
- `project`
- `assets`
- `compositions`
- `templates`
- `render_defaults`

## Example

```yaml
spec_version: "1.0"
project:
  id: "proj_001"
  name: "Intro"
assets:
  - id: "logo"
    type: "image"
    source: "assets/logo.png"
compositions:
  - id: "main"
    name: "Main"
    width: 1920
    height: 1080
    duration: 120
    frame_rate: 30
    layers:
      - id: "title"
        type: "text"
        name: "Title"
        start_time: 0
        in_point: 0
        out_point: 120
        transform:
          position: [960, 540]
        opacity: 1
render_defaults:
  output: "mp4"
```

The first code slice treats this shape as JSON input for the CLI and core parser, even though the example remains format-agnostic in intent.

## Composition contract

A composition should contain:

- `id`
- `name`
- `width`
- `height`
- `duration`
- `frame_rate`
- `background`
- `layers`

Layers are ordered from back to front.
A composition may be referenced as a precomp source by another layer.

## Layer contract

Every layer should contain:

- `id`
- `type`
- `name`
- `enabled`
- `start_time`
- `in_point`
- `out_point`
- `transform`
- `opacity`

Optional fields include `parent`, `blending`, `masks`, `matte`, `effects`, `audio`, `template_bindings`, and `metadata`.

## v0 layer types

The first version should target a narrow useful subset:

- `solid`
- `image`
- `text`
- `shape`
- `precomp`
- `null`
- `camera`

## Property form

A property should allow one of the following authoring forms:

- literal static value
- keyed animation
- expression reference
- external binding reference
- template override reference

## Versioning and compatibility

- `spec_version` is required and must be semver-like or otherwise explicitly parseable.
- New optional fields should be additive by default.
- Behavior changes that alter output must be versioned intentionally.
- Unknown fields should be rejectable in strict mode and warnable in lenient mode.

## Validation goals

Validation should happen before evaluation.
The spec layer should reject malformed scenes before rendering starts.

Validation should catch duplicate ids, missing references, invalid parent links, circular structural references, invalid property forms, impossible time intervals, and unsupported layer types.

## Forward compatibility rules

1. `spec_version` is mandatory.
2. Behavior changes must not silently alter older documents.
3. New versions should prefer additive evolution over semantic breakage.
4. Unknown fields may be warned on or rejected depending on validation mode.
