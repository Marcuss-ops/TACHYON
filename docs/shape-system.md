# Shape System

## Purpose

The shape system gives TACHYON a vector-native motion graphics foundation.
Without it, the engine would remain primarily an image compositor rather than a true motion design system.

## Core primitives

The shape layer architecture should support:

- paths
- fills
- strokes
- groups
- transforms per group

## Planned operators

- trim paths
- repeater
- boolean operations
- offset and expansion workflows later

## Architectural role

Shape layers are scene objects, not post-process effects.
They participate in transforms, hierarchy, animation, masks, effects, and compositing.

## Planned structure

```text
src/shape/
├── shape_layer.cpp
├── path.cpp
├── stroke.cpp
├── fill.cpp
├── trim_paths.cpp
├── repeater.cpp
└── boolean_ops.cpp
```

## Design rules

1. Shapes should remain resolution-independent.
2. Shape properties must be animatable and expression-addressable.
3. Shape evaluation and rasterization should stay separable.
4. The system should leave room for future material and lighting interaction in 2.5D or 3D workflows.
