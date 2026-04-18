# Minimum Vertical Slice

## Purpose

This document defines the first complete end-to-end target that should make TACHYON feel like a real engine rather than a collection of isolated subsystems.

The goal is not to imitate all of After Effects at once.
The goal is to prove the engine model with a focused slice that exercises scene spec parsing, property evaluation, transform resolution, compositing, and final media output.

## What this slice must prove

The first vertical slice should prove that TACHYON can:

1. load a real declarative scene spec
2. validate and normalize it
3. evaluate time-dependent properties deterministically
4. build explicit render work for each frame
5. raster and composite a small but meaningful scene
6. emit a final encoded output without browser infrastructure

If the slice cannot do those six things cleanly, the engine foundations are not yet real.

## Guiding constraint

The slice must stay compatible with low-end, CPU-first execution.
That means the slice should prefer correctness, determinism, and architectural clarity over effect count or flashy breadth.

## Scope of the first vertical slice

The initial end-to-end slice should include the following capabilities.

### 1. Scene spec ingestion

Required:

- JSON scene spec loading
- schema or structural validation
- normalization into internal engine state
- clear diagnostics for invalid references and unsupported values

### 2. Composition support

Required:

- one root composition render target
- explicit width, height, frame rate, duration
- deterministic frame iteration

Optional later:

- nested composition rendering
- time remap

### 3. Layer support

Required first layer kinds:

- solid
- image
- text
- null

Strongly recommended in the same slice if feasible:

- precomp

Layer kinds that can wait:

- camera
- shape
- light

### 4. Property system

Required:

- static properties
- numeric and vector keyframes
- linear interpolation
- hold interpolation
- deterministic property sampling at frame time

Can wait until the next slice:

- expressions
n- external data bindings
- advanced bezier easing

### 5. Transform and hierarchy

Required:

- anchor
- position
- scale
- rotation
- opacity
- parent-child hierarchy through null layers

This is one of the highest-value parts of the first slice because it proves that TACHYON is a scene engine rather than just a filter runner.

### 6. Text foundation

Required:

- single-style text layer
- deterministic text layout
- font loading through declared assets
- box-free basic text placement is enough for the first slice

Can wait:

- rich text
- paragraph layout complexity
- per-character animator system

### 7. Image and solid rendering

Required:

- image asset draw
- solid color layer draw
- alpha-aware compositing
- deterministic sampling policy

### 8. Compositing

Required:

- ordered layer compositing
- opacity application
- premultiplied alpha policy chosen and documented
- one normal blend path

Strongly recommended in the same slice if feasible:

- one track matte mode
- one mask mode

Can wait:

- adjustment layers
- advanced blend modes
- effect stacks deeper than one simple effect

### 9. Effect path

At least one real effect path should exist so the architecture does not avoid the problem.

Recommended first effects:

- fill
- blur

Only one is strictly necessary for the first slice, but the effect API should be designed so adding more later does not change the execution model.

### 10. Encoding/output

Required:

- PNG frame sequence output or equivalent image sequence
- MP4 output through an encoding backend

It is acceptable if the engine renders frames and a downstream encoder assembles them, as long as the boundary is explicit and deterministic.

## A concrete demo target

The first vertical slice should be able to render a small composition like this:

- background solid layer
- imported image layer
- null controller layer
- text layer parented to null
- animated position and opacity over time
- one simple matte or mask
- final MP4 output

If TACHYON can render that scene from a declarative spec end to end on a weak machine, the project becomes materially real.

## Capabilities that should not block this slice

The following should not delay the first vertical slice:

- full camera system
- 2.5D depth workflow
- lights and shadows
- advanced text animators
- complex shape boolean operations
- expression language
- data-driven repeaters
- distributed rendering
- plugin runtime
- UI or authoring environment

## Suggested subsystem deliverables

To make the first slice executable, the repo should eventually gain at least these implementation units:

- spec parser and validator
- scene normalization layer
- property sampling module
- transform and hierarchy evaluator
- text layout adapter
- image and solid raster path
- compositing pass
- output and encoding boundary
- golden scene fixtures for regression tests

## Test criteria

The first slice should not be considered done until it has:

- deterministic output on repeated runs
- fixture-based render tests
- validation failure tests
- property interpolation tests
- hierarchy transform tests
- at least one visual regression fixture

## Exit criteria

The vertical slice is complete when a developer can do the following:

1. write a small JSON scene spec
2. run a CLI render command
3. obtain a deterministic video output
4. verify the output with fixtures or image comparison

At that point, TACHYON has crossed the line from architecture concept to executable engine foundation.

## Guiding principle

The first slice should maximize engine truth, not feature count.
A smaller but semantically correct slice is worth more than a broad but ad hoc prototype.