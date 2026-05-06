# 2D Compositing

Documentation for the 2D layer stack, compositing pipeline, masks, effects, text rendering, motion blur, and color management.

Use the shared behavior model for the domain split:

- [Behavior Model](../10-architecture/behavior-model.md)

## Topics

- Shape layers
- Masks and mattes
- Effects
- Text rendering
- Motion blur
- Color management

## Related Docs

- [Text Module](../../src/text/README.md)

## Current Model

The 2D stack stays inside the existing scene and renderer boundaries:

- scene authoring defines the layer intent
- the evaluator resolves the layer at runtime
- renderer2d turns the evaluated layer into draw commands
- effects are applied through the effect host instead of ad hoc per feature

## How to Add a 2D Feature

1. Prefer extending an existing layer or effect path.
2. Add the minimum schema or builder change needed to describe the feature.
3. Add evaluator or renderer support in the existing pipeline.
4. Register the effect or helper in the current registry instead of creating a parallel path.
5. Cover the feature with a focused test in the same subsystem.

## Rules

- Do not introduce a second compositing pipeline.
- Do not hide layer logic inside unrelated subsystems.
- Do not bypass the existing effect host for a new 2D effect.
- Do not move 3D logic into 2D just to get a faster implementation.
