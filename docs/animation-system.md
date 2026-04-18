# Animation System

## Purpose

The animation system answers a simple question: what is the value of a property at time t?

This applies to transforms, opacity, camera properties, effect parameters, mask parameters, text selectors, template overrides, and more.

## Property sources

Every animatable property should eventually support three modes:

- static value
- keyframed value
- expression-driven value

These must be treated as part of one coherent system, not as disconnected implementations.

## Keyframes

Keyframes should support:

- linear interpolation
- hold interpolation
- bezier-style easing
- reusable ease presets

The goal is not just interpolation, but motion control.
Easy Ease-like behavior belongs here as a preset over more general easing primitives.

## Expressions

Expressions are a core engine concept, not a UI convenience.
A future expression subsystem should be able to read deterministic context such as:

- time
- layer properties
- parent transforms
- composition properties
- markers
- audio-derived values
- camera values
- seeded randomness

Expressions must be sandboxed, deterministic, and side-effect free.

## Time-dependent evaluation

The animation system should be usable by all major engine systems:

- scene evaluation
- cameras
- effects
- masks
- text animators
- template systems
- data bindings

## Design rule

Animation should be implemented as a shared engine service.
It should not be reimplemented independently inside text, effects, cameras, or layers.
