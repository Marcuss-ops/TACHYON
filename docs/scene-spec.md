# Scene Spec

## Purpose

This document defines the first concrete external contract for authored scenes in TACHYON.
The goal is not to freeze every future capability up front, but to establish a stable enough project shape that engine code can start against real inputs instead of vague concepts.

A scene spec is the reusable authored blueprint.
A render job is the runtime execution request that selects a scene spec and provides overrides, bound data, and output settings.

## Design goals

The first scene spec should be:

- deterministic
- declarative
- versioned
- serializable
- validation-friendly
- easy to diff in Git
- expressive enough for real motion templates
- narrow enough to support a small initial engine

## Format direction

The initial format should be JSON.
A future alternative encoding is allowed, but the conceptual model should remain the same.

Top-level rules:

1. Every scene spec must declare a format version.
2. IDs must be explicit and stable.
3. Cross-references must use IDs, not implicit position.
4. Authoring-time defaults should be made explicit during validation or normalization.
5. The renderer must consume normalized internal state, not raw source text.

## Top-level structure

A minimal scene spec should support the following top-level sections:

- `specVersion`
- `project`
- `assets`
- `compositions`
- `templates`

### Example skeleton

```json
{
  "specVersion": "1",
  "project": {
    "id": "project_main",
    "name": "Promo Template",
    "frameRate": 30,
    "width": 1920,
    "height": 1080,
    "durationFrames": 300,
    "workingColorSpace": "srgb"
  },
  "assets": [],
  "compositions": [],
  "templates": {
    "publishedProperties": []
  }
}
```

## Project section

The `project` section defines shared defaults and global authoring assumptions.

Minimum recommended fields:

- `id`
- `name`
- `frameRate`
- `width`
- `height`
- `durationFrames`
- `workingColorSpace`

Optional future fields may include:

- pixel aspect ratio
- audio sample rate
- drop-frame timecode policy
- default motion blur settings
- compatibility profile

## Assets

The `assets` section declares reusable imported resources.
This keeps layer declarations smaller and makes validation more explicit.

Initial asset kinds should be limited to:

- image
- image_sequence
- video
- audio
- font
- nested_composition_reference

Each asset should have:

- `id`
- `kind`
- `source`

Common optional metadata:

- `label`
- `expectedWidth`
- `expectedHeight`
- `expectedDurationFrames`
- `colorSpace`
- `alphaMode`

### Example asset

```json
{
  "id": "img_logo",
  "kind": "image",
  "source": {
    "path": "assets/logo.png"
  },
  "colorSpace": "srgb",
  "alphaMode": "premultiplied"
}
```

## Compositions

A composition is the main reusable timeline container.
Every renderable scene should have at least one composition.

Each composition should define:

- `id`
- `name`
- `width`
- `height`
- `durationFrames`
- `frameRate`
- `layers`

Optional future fields:

- background color
- local working space override
- motion blur settings
- renderer hints

### Example composition

```json
{
  "id": "comp_main",
  "name": "Main",
  "width": 1920,
  "height": 1080,
  "durationFrames": 300,
  "frameRate": 30,
  "layers": []
}
```

## Layer model

Layers are ordered visual or structural elements inside a composition.
The initial spec should keep the first layer family small and well defined.

Recommended initial layer kinds:

- `solid`
- `image`
- `text`
- `shape`
- `null`
- `camera`
- `precomp`

Every layer should have a stable identity and shared envelope fields.

### Common layer fields

- `id`
- `name`
- `kind`
- `startFrame`
- `durationFrames`
- `enabled`
- `parentId`
- `transform`

Optional but important shared fields:

- `opacity`
- `blendingMode`
- `masks`
- `trackMatte`
- `effects`
- `collapse` or equivalent future precomp behavior

## Transform group

Every visual or structural layer except where explicitly disallowed should expose a transform group.

Initial transform fields:

- `anchor`
- `position`
- `scale`
- `rotation`
- `opacity`

Future expansion may include:

- separate x y z rotation
- skew
- orientation
- auto-orient policies
- 3D layer toggle

### Example transform group

```json
{
  "anchor": { "type": "static", "value": [0, 0] },
  "position": { "type": "static", "value": [960, 540] },
  "scale": { "type": "static", "value": [100, 100] },
  "rotation": { "type": "static", "value": 0 },
  "opacity": { "type": "static", "value": 100 }
}
```

## Property representation

The scene spec should not store raw numbers only.
It should express property source kind explicitly.
This aligns authored data with the property model and keeps runtime evaluation deterministic.

The first property envelope should support:

- static
- keyframed
- expression
- binding

### Static property example

```json
{
  "type": "static",
  "value": [960, 540]
}
```

### Keyframed property example

```json
{
  "type": "keyframed",
  "valueType": "number",
  "keyframes": [
    { "frame": 0, "value": 0, "interp": "linear" },
    { "frame": 20, "value": 100, "interp": "ease_in_out" }
  ]
}
```

## Layer kind specifics

### Solid layer

Required fields:

- `width`
- `height`
- `color`

### Image layer

Required fields:

- `assetId`

Optional fields:

- fit mode
- sampling mode
- preserve alpha mode override

### Text layer

Required fields:

- `text`
- `style`

Initial text style fields:

- `fontFamily`
- `fontSize`
- `fontWeight`
- `fillColor`
- `tracking`
- `leading`
- `alignment`

### Shape layer

Initial shape support should remain small:

- rectangle
- ellipse
- path

Initial shape graphics support:

- fill
- stroke
- trim paths in a later step

### Null layer

A null layer is non-rendering and exists for hierarchy, rigging, and control semantics.

### Camera layer

Initial camera fields should support a motion-graphics-focused camera rather than a full DCC camera model.

Initial camera fields:

- `projection`
- `position`
- `pointOfInterest`
- `fov` or focal length
- `nearClip`
- `farClip`

### Precomp layer

Required fields:

- `compositionId`

Optional future fields:

- time remap
- collapse transformations policy
- local frame mapping rules

## Masks

Masks should be first-class structures on layers, not effect hacks.

Initial mask fields:

- `id`
- `mode`
- `shape`
- `opacity`
- `feather`
- `expansion`

Initial mask modes may include:

- add
- subtract
- intersect

## Track mattes

Track mattes should be explicit references, not inferred from layer adjacency.
That keeps the spec more stable and machine-friendly.

Initial matte fields:

- `sourceLayerId`
- `mode`

Initial matte modes:

- alpha
- alpha_inverted
- luma
- luma_inverted

## Effects

Effects should be represented as typed parameterized blocks.
The initial spec should not attempt to standardize a giant effect catalog.

Each effect should define:

- `id`
- `kind`
- `enabled`
- `params`

Initial candidate effect kinds for the first engine slice:

- transform
- blur
- fill
- tint
- levels

## Templates and published properties

A scene spec may publish a small set of author-approved override points.
This is how a reusable authored scene becomes a template.

Each published property should define:

- `id`
- `target`
- `valueType`
- `defaultValue`
- `label`
- `constraints`

### Example published property

```json
{
  "id": "headline_text",
  "label": "Headline",
  "target": {
    "layerId": "title_01",
    "propertyPath": "text.source"
  },
  "valueType": "string",
  "defaultValue": "Hello world"
}
```

## Validation requirements

The validator for spec version 1 should at minimum check:

1. required top-level sections
2. unique IDs within each namespace
3. valid references between compositions, layers, assets, and templates
4. legal property envelope shape
5. legal frame ranges and non-negative durations
6. supported layer kinds and effect kinds
7. type compatibility between template targets and default values

## Normalization requirements

Before runtime evaluation, the engine should normalize scene specs into an internal form.
Normalization may:

- fill defaults
- canonicalize property envelopes
- resolve shorthand forms
- precompute IDs and lookup tables
- attach inferred project defaults

Normalization must not introduce hidden nondeterminism.

## Non-goals for version 1

The first scene spec does not need to solve everything.
It should explicitly avoid overreach.

Version 1 should not try to fully solve:

- full 3D mesh scenes
- simulation systems
- plugin-defined arbitrary spec fragments
- full expression language design
- distributed render orchestration
- every AE feature family

## Guiding principle

The first scene spec should be strong enough that multiple developers can implement the same scene semantics and arrive at the same evaluated state.
If a scene document is too ambiguous for that, the spec is still too soft.