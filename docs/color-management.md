# Color Management

## Purpose

Correct compositing and effects behavior depend on a coherent color pipeline.
Without an explicit color management strategy, blends, glows, shadows, and downstream matching will become inconsistent.

## Core goals

- define a working color space
- distinguish display representation from working representation
- support correct blending behavior
- leave room for future OCIO integration
- define linear-light composition as the default internal mode
- specify output transforms per target format
- keep gamma handling explicit

## Initial operating model

- decode assets into a known working space
- convert scene operations to linear light
- composite and evaluate effects in the working space
- transform only at output boundaries
- keep HDR/SDR output transforms explicit

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
5. The engine should treat display transform as a final step, not as a hidden property of the compositor.
