# Scene Model

## Purpose

The scene model defines the world that TACHYON evaluates and renders.
It must support motion graphics, compositing, 2D layers, 3D layers, cameras, null rigs, and precompositions without depending on editor UI concepts.

## Core entities

### Composition
A composition is a timed container with dimensions, duration, frame-rate assumptions, layer stack, and optional active camera.
A composition may eventually be nested inside another composition through a precomp layer.

### Layer
A layer is a time-bounded scene node.
Layers may be renderable, structural, or control-only.

Planned layer families:

- SolidLayer
- ImageLayer
- TextLayer
- ShapeLayer
- NullLayer
- CameraLayer
- LightLayer
- PrecompLayer
- AdjustmentLayer
- MeshLayer

### Node hierarchy
Layers participate in parent-child hierarchies.
This enables rigs, grouped motion, null-driven control systems, camera parenting, and reusable transforms.

### Space mode
Each layer should explicitly declare whether it exists in:

- screen space 2D
- world space 3D

Parallax, camera motion, and perspective should emerge from this distinction naturally.

### Structural layers
Some layers are not directly visible but still matter to the engine.
Examples include:

- null layers for rigging
- cameras for viewing the world
- lights for future shading
- adjustment layers for downstream effect application

## First-class scene concepts

TACHYON should treat the following as first-class architectural concepts from the beginning:

- compositions
- layers
- parenting
- transforms
- cameras
- point of interest
- precomps
- adjustment layers
- masks and mattes
- shape content
- effect stacks
- published template properties

## Evaluation model

The scene model should be evaluated at time t into a resolved state.
That resolved state is what downstream systems consume.
The renderer should not operate directly on raw scene spec structures.

## Design rule

The scene model is not the renderer.
It describes what exists, how objects relate, and what should be true at a given time.
Pixel generation belongs downstream.
