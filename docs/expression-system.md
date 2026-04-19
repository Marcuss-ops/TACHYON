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

## Lua direction

Lua is the preferred expression language candidate for TACHYON.

If the scene spec uses Lua as the expression surface, then expressions are just Lua functions over a constrained context instead of a custom parser with a bespoke AST and evaluator.

That buys:

- a small, well-understood language
- deterministic sandboxing options
- simpler authoring for procedural motion
- easier embedding in a native engine
- lower implementation risk than inventing a new expression language

The Lua environment should still be restricted:

- no filesystem access
- no network access
- no unsafe native bindings
- no hidden global state
- deterministic helpers only

## Why this matters

Expression systems become expensive when the engine invents yet another mini-language, parser, and runtime.
Lua lets TACHYON keep the expression slot powerful without turning the renderer core into a language project.

## Architectural rule

Expressions are not a patch on top of animation.
They are part of the core property evaluation model.

## Implementation direction

The engine should keep the expression slot separate from the renderer and compatible with a sandboxed Lua runtime.
The exact host embedding can be chosen later, but the architectural slot must already exist.
