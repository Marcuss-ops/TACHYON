# Expression System

## Why expressions matter

A motion graphics engine is not truly programmatic if every property must be authored as fixed keyframes.
Expressions turn the engine into a deterministic time-based dataflow system where properties can derive values from other properties and scene context.

## Property model

Every animatable property should eventually support one of these sources:

- static value
- keyframed value
- expression result

This should apply uniformly to transforms, opacity, camera settings, effect parameters, mask controls, text selectors, and template-exposed parameters.

## Required properties of the system

Expressions must be:

- deterministic
- sandboxed
- side-effect free
- reproducible
- safe to evaluate during rendering

## Planned expression context

A future expression evaluator should be able to read:

- time
- current layer properties
- parent properties
- composition metadata
- markers
- camera values
- audio-derived values
- seeded randomness
- published template properties
- bound external data values

## Architectural rule

Expressions are not a patch on top of animation.
They are part of the core property evaluation model.

## Implementation direction

The engine should leave room for an internal expression VM or sandboxed runtime.
The exact execution technology can be chosen later, but the architectural slot must already exist.
