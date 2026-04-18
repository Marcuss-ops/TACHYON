# Execution Model

## Purpose

This document defines how TACHYON should think about rendering internally.
The engine should not be treated as a timeline player that walks through a stack of UI-like layers one frame at a time.
It should be treated as a deterministic compute engine that evaluates scene state and produces pixel buffers.

## Core idea

A renderable scene is ultimately a time-indexed computation.
At a high level:

- scene inputs are resolved
- properties are evaluated at time t
- render operations are derived
- pixel buffers are produced
- encoded output is emitted later

This means the engine should be designed around evaluated dataflow and render kernels, not around browser-style document metaphors.

## From layer metaphor to execution model

Externally, the project may still expose concepts such as compositions, layers, cameras, and effects because they are important authoring and scene-model concepts.
Internally, however, those concepts should converge toward streams of resolved parameters and render operations.

A layer should not force a UI-shaped implementation model.
At execution time, it becomes a source of evaluated properties such as:

- transform
- visibility
- opacity
- geometry or image source
- text state
- mask state
- effect inputs

Those values feed render and compositing work.

## High-level execution stages

1. parse and validate input specs
2. resolve template overrides and data bindings
3. evaluate animated and expressed properties at time t
4. build frame-local render work
5. execute render passes
6. composite intermediate results
7. submit final frame to encoder or frame sink

## Design rules

1. Scene concepts and execution concepts should remain separable.
2. Rendering must operate on evaluated state, not raw authoring descriptions.
3. The execution model must remain compatible with temporal parallelism and deterministic caching.
4. Encoding must remain downstream of image generation.

## Guiding principle

TACHYON should be thought of as a temporal pixel compute engine with a scene model, not as a browser-like playback stack.
