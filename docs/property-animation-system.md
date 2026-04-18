# Property and Animation System Spec

## Purpose

This document defines the canonical model for animatable values in TACHYON.
It exists to ensure that transforms, camera settings, effect parameters, mask controls, text controls, and template-driven overrides all participate in one coherent evaluation model.

## Core idea

A property is not just a number stored on a layer.
A property is a typed value source that can be sampled at time t and traced through explicit dependencies.

## Property source categories

A property may eventually resolve from one of these categories:

- static value
- keyframed value
- expression-driven value
- externally bound value
- derived or linked value

## Required operations

Every animatable property should conceptually support:

- type identity
- value sampling at time t
- dependency enumeration
- deterministic hashing for caching and invalidation
- diagnostics when resolution fails

## Animation requirements

The animation system should support at least:

- keyed values
- hold interpolation
- eased interpolation
- continuous curve-based interpolation where appropriate
- time remap compatibility
- expression interaction

## Dependency role

A property should be able to declare which other values it depends on.
That allows cycle detection, incremental recompute, caching, and stable evaluation order.

## Design rules

1. The same property model should serve scene values, effect parameters, camera values, masks, and template-exposed controls.
2. Sampling must be deterministic.
3. Property resolution must be explicit enough to support diagnostics and cache keys.
4. Expressions and bindings must be first-class branches of the same model, not hacks layered on top.

## Guiding principle

If a system parameter cannot participate in the property model, it is not yet integrated into the engine in a scalable way.
