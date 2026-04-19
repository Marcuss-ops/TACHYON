# 2D and 3D Compositing Boundary

## Purpose

This document defines the boundary between TACHYON's compositing system and its future 3D rendering backend.

## Why this exists

Without a strict boundary, the engine risks becoming architecturally confused.

Some features belong naturally in the compositor.
Some belong naturally in the 3D renderer.
Some require cooperation between both.

This document defines those categories.

## Compositor-owned work

The compositor should own:

- layer ordering
- precomp resolution
- masks and mattes
- blend modes
- adjustment layers
- image-space effects
- text and vector composition
- 2D transforms
- 2.5D layer stacking where applicable
- pass combination
- final alpha semantics

## 3D renderer-owned work

The 3D backend should own:

- ray intersection and visibility
- physically based shading
- direct and indirect lighting
- shadows
- reflections
- refractions
- true geometric depth of field
- geometric motion blur
- mesh visibility and instancing
- material evaluation

## Shared responsibilities

Some systems require explicit cooperation:

- cameras
- motion blur policy
- color management
- depth-aware effects
- depth-aware compositing
- render pass selection

## Frame construction model

A composition may contain:

- pure 2D layers
- pure 3D layers
- 3D precomps
- 2D overlays above 3D work
- 2D effects using 3D-derived buffers

The final frame should be constructed by the compositor using explicit pass inputs, not by allowing the 3D renderer to bypass the composition model.

## Recommended pass handoff

A 3D layer or 3D precomp should be able to hand off:

- beauty
- alpha
- depth
- normal
- albedo
- motion vectors
- object id

The compositor may then apply:

- adjustment layers
- color treatment
- depth-aware effects
- matte operations
- overlays
- final encode path

## Text and vector rule

Text and vector graphics should remain compositor-native by default.

Even when 3D content exists in the frame, text and vector layers should not be forced through a 3D renderer unless explicitly authored as 3D geometry.

## Extrusion rule

Extruded text and extruded shapes are exceptions.

These authored objects should be promoted into the 3D backend as geometry while preserving their identity at the scene level.

## Camera rule

The scene model should define cameras once.

Backends should consume resolved camera data from the scene engine rather than reinterpreting camera semantics independently.

## Non-goal

The engine should not treat the compositor as a thin wrapper around the 3D backend. The compositor remains the system that owns the final meaning of the frame.
