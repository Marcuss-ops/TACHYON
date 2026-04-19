# Expression Runtime

## Purpose

This document defines how expression execution should fit into TACHYON without replacing the declarative scene contract.

## Position

Expressions should be treated as bounded runtime logic attached to properties, not as a replacement for the scene spec.

## Why this matters

The scene spec is intended to be:

- declarative
- versioned
- explicit
- validatable
- deterministic
- reusable across render jobs

Those goals are weakened if the entire scene becomes arbitrary executable code.

## Recommended split

### Scene spec

The scene spec should remain the canonical authored data contract.

It defines:

- compositions
- layers
- assets
- cameras
- lights
- materials
- properties
- animations
- published template parameters

### Expression runtime

Expressions should provide limited procedural behavior for properties.

They may be used for:

- property linking
- selectors
- rig-like logic
- math utilities
- time-based formulas
- audio-driven mappings
- small authored automation patterns

## Why Lua is a strong candidate

Lua is attractive because it is:

- small
- embeddable
- easy to integrate with C++
- suitable for constrained runtimes
- capable enough for property logic

## Why Lua should not be the whole scene format

Using Lua as the canonical full-scene format would make it harder to preserve:

- schema validation
- stable diffs
- static analysis
- dependency extraction
- cache key derivation
- bounded execution guarantees
- clear authored versus computed data boundaries

## Proposed expression contract

An expression should:

- target one property
- execute in a sandboxed environment
- receive explicit inputs
- return one value of a known type
- avoid hidden global mutation
- avoid I/O
- avoid filesystem access
- avoid network access
- avoid nondeterministic time access

## Expression inputs

The runtime may expose:

- current time
- frame index
- composition information
- layer information
- named published parameters
- referenced property values
- utility math functions
- selected audio features when enabled

## Expression outputs

Expressions should only return values compatible with the target property type, such as:

- scalar
- vector
- color
- boolean
- string where allowed

## Dependency model

An expression should declare or allow the engine to derive its dependencies.

At minimum, the engine must know whether an expression depends on:

- time
- another property
- a published parameter
- audio feature inputs

This is required for invalidation and caching.

## Determinism model

The expression runtime must define:

- language version
- standard library subset
- random seed policy
- floating-point expectations
- error behavior
- time access rules

## Failure model

An expression failure should never crash the engine silently.

The engine should surface structured diagnostics and define whether the property falls back, clamps, or invalidates the render.

## Recommended first milestone

The first expression milestone should stay small:

- numeric math
- vector math
- time-based formulas
- property linking
- simple selectors
- bounded audio mappings

That is enough to unlock real authoring value without building a large scripting platform.
