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

## Recommended execution choice

Lua is a strong fit for the first expression runtime because it is lightweight, embeddable, easy to sandbox, and mature enough for host-driven extension.

That makes it attractive for:

- property expressions
- selector logic
- utility math helpers
- small template-side scripting hooks

## Why Lua

Lua fits TACHYON better than a browser-style scripting stack because it is:

- small and easy to embed in a native C++ engine
- fast enough for expression workloads without dragging in a large runtime
- simple to sandbox compared with broader general-purpose environments
- well suited to host-controlled APIs and deterministic evaluation models

## Important boundary

The canonical scene specification should remain declarative and versioned.
Lua should not replace the scene spec as the primary project format.

That means:

- scene structure should stay in a validated data format
- expressions may be authored as Lua snippets or references
- host APIs exposed to Lua should remain narrow and deterministic
- render output must not depend on unrestricted user scripting behavior

Keeping this boundary protects validation, compatibility, caching, and tooling.

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

## First host API direction

The first expression host surface should stay small:

- numeric and vector math
- time access
- property reads
- seeded random
- clamp, remap, smoothstep, easing helpers
- text selector and range selector helpers later

## Non-goals for the first version

- unrestricted file I/O
- network access
- mutation of scene topology during render
- hidden global state
- broad standard-library exposure

## Implementation direction

The engine should reserve a stable expression slot in the property pipeline and start with a deliberately narrow Lua host API rather than a broad custom language or an unsafe general scripting environment.
