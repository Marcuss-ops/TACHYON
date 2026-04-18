# Text Animator System

## Purpose

Text animation in a motion graphics engine cannot stop at block-level layout.
To achieve professional title behavior, the engine must support glyph-level and range-based animation.

## Core model

A text layer should be able to own one or more text animators.
Each animator may influence a selected subset of glyphs through range selectors or equivalent selection logic.

## Per-glyph properties

The architecture should support per-glyph animation of:

- position
- scale
- rotation
- opacity
- fill and styling parameters
- tracking and spacing-related controls

## Selection logic

Selectors should eventually support:

- index ranges
- percentage ranges
- seeded randomness
- temporal offsets
- easing based on glyph order or selector progression

## Architectural role

The text animator system sits above text layout and below final rasterization.
It transforms shaped text runs into animated glyph states at time t.

## Planned structure

```text
src/text/
├── text_animator.cpp
├── range_selector.cpp
└── text_3d_extrude.cpp
```

## Design rules

1. Text animation must remain deterministic.
2. Glyph-level state must be expression-addressable and animatable.
3. Layout and animation must remain separable so text can be re-evaluated cleanly.
4. The system should support both 2D and future 3D text workflows.
