# Color Management

## Purpose

Correct compositing and effects behavior depend on a coherent color pipeline.
Without an explicit color management strategy, blends, glows, shadows, and downstream matching will become inconsistent.

## Core goals

- define a working color space
- distinguish display representation from working representation
- support correct blending behavior
- leave room for future OCIO integration

## Architectural role

Color management is a cross-cutting concern.
It influences rasterization, compositing, effect execution, and final output conversion.
It must not be postponed until after the effect stack exists.

## Planned structure

```text
src/color/
├── color_space.cpp
├── ocio_transform.cpp
└── working_space.cpp
```

## Design rules

1. Blending behavior must be defined against the working space.
2. The engine should leave room for linear-light workflows.
3. Output transforms should remain explicit.
4. The system should support deterministic conversion behavior across environments.
