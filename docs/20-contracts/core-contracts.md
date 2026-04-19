# Core Contracts

## Purpose

This document locks the foundational contracts that the rest of TACHYON should build on.

These rules exist to prevent expensive rewrites later.
They should be treated as engine-level agreements rather than local implementation details.

## 1. Space conventions

TACHYON should distinguish clearly between raster space, composition space, and 3D world space.

### Raster space
- origin at top-left
- x increases to the right
- y increases downward
- integer pixel addressing refers to raster locations, not authoring semantics

### Composition 2D space
- origin at top-left of the composition
- x increases to the right
- y increases downward
- distances are expressed in composition pixels

### 3D world space
- right-handed world
- x increases to the right
- y increases upward
- camera local forward points along negative z
- projection maps world output into raster space after camera evaluation

## 2. Units

### Time
- internal time is measured in seconds
- composition frame rate must be stored as an explicit rational or equivalent stable representation
- frame index and frame time are related but not interchangeable

### Distance
- 2D properties use composition pixels
- camera focal settings should use explicit physical-style units such as millimeters where relevant
- 3D transforms use scene units, with the default camera conventions documented so authored results are predictable

## 3. Time semantics

The engine must define one stable interpretation for layer timing.

Recommended first rule set:

- composition time is the global time domain for a render pass
- layer local time is composition time offset by layer start time and any later time-domain modifiers
- in-point and out-point control visibility but do not change the meaning of authored keyframe times by themselves
- precompositions may evaluate under their own local time domain and optional remapping

## 4. Alpha contract

The engine should use one explicit internal alpha model.

Recommended first contract:

- internal render surfaces use premultiplied alpha
- alpha conversions occur only at explicit boundaries
- alpha behavior must be documented for decode, compositing, effects, and encode handoff

Alpha ambiguity is not acceptable in a deterministic compositing engine.

## 5. Color contract

Recommended first baseline:

- internal compositing occurs in linear light
- the initial working primaries should be linear sRGB or equivalent Rec.709-aligned primaries unless later expanded
- internal precision should prefer 32-bit float in compositing-critical paths
- output transforms must be explicit rather than hidden in backend behavior

## 6. Determinism and seeded randomness

Every source of intentional randomness must derive from explicit seeds.

Recommended first seed rule:

- render job has a stable root seed
- feature-local seeds derive from a stable hash of root seed plus scope identifiers such as composition id, layer id, property path, frame index, and sample index
- hidden time-based or process-based randomness is forbidden

## 7. Layer identity and ordering

The engine should treat layer identity as stable data, not incidental position in memory.

That means:

- every layer must have a stable id
- ordering from back to front is explicit in authored compositions
- references such as parenting, mattes, and bindings should resolve through ids or similarly stable references

## 8. Render identity boundaries

The engine must distinguish between:

- authored scene identity
- render job identity
- visible-output identity

Visible-output identity must include any setting that can change pixels or audio, including output-affecting quality policies where relevant.

## 9. Error and diagnostics contract

The engine should expose structured diagnostics.

At minimum, diagnostics should distinguish:

- validation failures
- evaluation failures
- render failures
- encode failures
- resource pressure or capacity failures

## 10. Compatibility rule

Any change that can alter visible output for the same scene spec and render job should be treated as compatibility-significant.

That includes changes to:

- alpha behavior
- color transforms
- shutter or sampling semantics
- layer timing interpretation
- blend mode behavior
- effect evaluation order where output changes

## 11. What should remain flexible for now

This document intentionally does not lock every future feature detail.
It should lock only the contracts that are expensive to reverse.

Examples of flexible areas for now:

- exact effect catalog breadth
- future render backend count
- long-term material model richness
- advanced simulation features

## Guiding principle

TACHYON should freeze the engine rules that affect every subsystem early.
That is how the project stays programmable, deterministic, and maintainable while growing toward serious compositing quality.
