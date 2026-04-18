# Composition and Layer Model

## Purpose

This document defines the composition and layer model that gives TACHYON its motion graphics structure.

## Core idea

A composition is a time-bounded visual container.
A layer is a time-bounded visual or control node inside a composition.

## Composition rules

1. A composition has its own dimensions, duration, frame rate, and layer stack.
2. A composition may be rendered directly or referenced as a precomp.
3. Composition boundaries are semantic boundaries for caching, evaluation, and compositing.

## Layer categories

The initial model should distinguish between:

- visual layers
- control layers
- structural layers

### Visual layers

- solid
- image
- text
- shape
- precomp

### Control layers

- null
- camera

## Layer ordering

Layers are evaluated in declared stack order and composited from back to front unless a compositing feature changes the render plan.

## Parenting

Parenting affects transform inheritance, not ownership.
A child keeps its own timeline and layer identity.

## Precomps

A precomp layer references another composition.
A precomp is not just a grouping trick.
It is a real composition boundary that may become its own render pass and cache unit.

## Null layers

Null layers are non-rendering control nodes.
They exist to carry transforms, rigs, and reusable motion relationships.

## Cameras

The camera model should remain focused on motion graphics needs.
The first implementation should support 2.5D scene work rather than a full general-purpose 3D DCC model.

## Adjustment layers

Adjustment layers should be supported as first-class composition semantics rather than hidden effect hacks.
They modify the rendered result of eligible layers beneath them within their active range.

## Track mattes and masks

Masks operate on a layer's own contribution.
Track mattes use another layer or pass as a visibility source.
These must be explicit compositing concepts.

## Coordinate spaces

The model should name and preserve at least:

- layer space
- parent space
- composition space
- world space
- camera space

## Design constraint

The layer model should stay rich enough for After Effects-like motion workflows while remaining narrow enough to be implemented deterministically on CPU-first infrastructure.
