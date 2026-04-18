# Property Model

## Purpose

The property model defines how values exist inside TACHYON.
This is one of the core documents for turning the engine from a static scene renderer into a temporal dataflow system.

## Core idea

A property should not be modeled as only a literal value.
It should be able to resolve from multiple sources through a unified evaluation model.

## Property sources

A property may eventually resolve from one of the following categories:

- static value
- keyframed value
- expression-driven value
- externally bound value
- derived value from template overrides or scene relationships

## Architectural role

The property model sits underneath animation, expressions, templates, data binding, camera settings, effect parameters, mask settings, and text animation.
It is one of the core contracts of the engine.

## Evaluation concept

At evaluation time, a property should become a concrete typed value for frame or time t.
The renderer must consume resolved values, not unresolved authoring forms.

## Design rules

1. Property resolution must be deterministic.
2. The source kind of a property must be explicit.
3. Expressions and bindings must participate through the same property evaluation pipeline rather than through special-case hacks.
4. The property model must remain type-aware so caching and diagnostics stay meaningful.

## Guiding principle

TACHYON should evaluate properties in time and then render the result.
That is a deeper model than simply reading numbers from a scene file.
