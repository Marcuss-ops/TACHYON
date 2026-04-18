# Masking and Matte System

## Purpose

Masking and track matte support are core compositing features, not optional visual extras.
They are required for serious title systems, reveals, lower thirds, selective effects, and adjustment workflows.

## Mask model

A renderable layer may own a stack of vector masks.
Each mask should support:

- add
- subtract
- intersect
- difference
- invert
- feather
- expansion

## Track matte model

A layer may reference another layer or intermediate result as a matte source.
Planned matte modes include:

- alpha matte
- alpha inverted matte
- luma matte
- luma inverted matte

## Adjustment layers

The architecture must also support adjustment-style layers that apply effects to the composite beneath them instead of rendering their own source content.

## Architectural role

Masks and mattes participate in compositing and render planning.
They must not be treated as one-off hacks inside individual layer draw functions.

## Planned structure

```text
src/mask/
├── mask_stack.cpp
├── mask_shape.cpp
└── feather.cpp

src/composite/
├── matte_resolver.cpp
└── track_matte.cpp
```

## Design rules

1. Mask behavior must be deterministic and animatable.
2. Matte dependencies must be explicit in the render graph.
3. Expansion and feathering must integrate with caching and pass planning.
4. Adjustment-style operations must remain compositing concepts, not special parser cases.
