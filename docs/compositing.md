# Compositing

## Purpose

TACHYON should be treated as a compositing engine, not just a layer blitter.
That means frame construction may require multiple passes, intermediate targets, mattes, depth-aware operations, effect application, and precomposition resolution.

## Core compositing concepts

### Layer stack
A composition contains ordered layers, but final output may require more than a simple top-to-bottom draw.

### Precomps
A precomp is a composition rendered as a layer inside another composition.
It should remain a first-class concept, not a manual workaround.

### Adjustment layers
An adjustment layer does not introduce new source pixels.
Instead, it applies effects to the composed result beneath it.

### Track mattes
Track mattes should be treated as part of the compositing system.
Expected conceptual modes include:

- alpha matte
- alpha inverted matte
- luma matte
- luma inverted matte

### Masks
Masks belong at the layer and compositing boundary.
They affect what part of a layer contributes to downstream compositing.

### Effect stacks
Each layer may carry an effect stack.
This implies a layer-oriented effect host and potentially a broader effect graph later.

## Render graph

Compositing should be expressed through a render graph or pass graph.
Typical pass families may include:

- world pass
- overlay pass
- mask pass
- matte pass
- precomp pass
- depth-aware pass
- final composite pass

## Design rule

Compositing logic must not be hidden in encoding or in ad hoc layer code.
It needs its own architectural home so that mattes, masks, precomps, effects, and optical passes can evolve cleanly.
