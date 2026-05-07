---
Status: Canonical
Last reviewed: 2026-05-07
Owner: Core Team
Supersedes: N/A
---

# Graphics Extension Contract

This document defines the canonical rule for adding new graphics behavior to Tachyon.

## The Rule

Every new graphics feature must enter the engine through the same path:

```text
Spec -> Validate -> Resolve -> Evaluate -> Intent -> Render
```

That means:

- authoring data stays in `Spec` or preset params
- domain registries resolve names to concrete specs
- evaluation normalizes the request into neutral runtime state
- the renderer consumes a final intent or evaluated state
- renderer code does not interpret authoring data directly

## What This Contract Is For

Use this contract whenever you add or extend:

- backgrounds
- text
- transitions
- effects
- any future graphics primitive that produces renderable output

## Canonical Shape

The implementation should follow this shape:

1. **Spec**
   - pure data
   - explicit fields
   - no renderer dependencies
2. **Registry / Preset**
   - maps names to known behaviors
   - stays in the authoring or presets layer
3. **Resolve / Evaluate**
   - validates input
   - resolves asset references
   - produces diagnostics when something is missing or invalid
4. **Intent / Runtime State**
   - neutral contract for the renderer
   - typed values only
   - no hidden pointers or renderer-local assumptions
5. **Render**
   - consumes the final neutral contract
   - performs only the actual drawing work

## Reference Order

If you are learning how to add a new graphics feature, read the systems in this order:

1. `docs/10-architecture/behavior-model.md`
2. `docs/10-architecture/presets-contract.md`
3. `docs/30-scene-and-animation/backgrounds.md`
4. `docs/30-scene-and-animation/transitions.md`
5. `docs/40-2d-compositing/README.md`

Background is the cleanest reference implementation today.

## Background As The Model

Background already shows the desired pattern:

- authoring enters through `BackgroundSpec` and background presets
- preset resolution stays in the presets layer
- the scene keeps explicit intent
- no renderer-specific shortcut is required to understand the request

This is the model to copy when a new graphics behavior is added.

## Rules

- Do not add a second pipeline for the same behavior.
- Do not let the renderer infer authoring intent from ad hoc fields.
- Do not introduce hidden defaults that change behavior silently.
- Do not couple `core` evaluation code to renderer-specific headers.
- Do not use compatibility bridges as the permanent design.
- Do not add a new graphics feature without a focused test at the owning boundary.

## Practical Test

When reviewing a proposed graphics feature, ask:

1. Can it be expressed as data first?
2. Can it be resolved without touching the renderer?
3. Does evaluation produce a neutral state or intent?
4. Can the renderer consume that state without special-casing the authoring format?

If any answer is no, the design is too coupled.

## Recommended Extension Pattern

For a new feature, prefer this order:

1. add the minimal `Spec`
2. add or extend the owning registry or preset
3. resolve the request into neutral state
4. add renderer support only for the neutral contract
5. add a boundary check if the feature crosses subsystem lines

## Why This Matters

This contract keeps Tachyon easy to extend because each feature has:

- one owning domain
- one explicit data model
- one neutral runtime contract
- one renderer responsibility

That reduces hidden coupling and makes regressions easier to detect.
