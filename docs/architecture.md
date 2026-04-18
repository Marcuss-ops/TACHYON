# Architecture Overview

## Architectural goal

TACHYON should be built as a scene, compositing, and temporal dataflow engine, not as a single renderer file and not as a browser replacement glued together from unrelated parts.

The architecture must make room for:

- compositions
- layers
- 2D and 3D transforms
- parenting and rigging
- camera systems
- animation evaluation
- expressions and bindings
- a unified property model
- shape layers
- masks and mattes
- effect graphs
- text animation systems
- color management
- deterministic caching
- native encoding
- explicit execution and parallel scheduling models

## Major subsystems

### 1. Spec system
Responsible for reading and validating external scene descriptions.
This layer defines the input contract and the public project shape.

### 2. Scene model
Responsible for internal representations of scenes, compositions, nodes, layers, precomps, nulls, lights, and cameras.
This is the conceptual heart of the engine.

### 3. Property model
Responsible for defining how values exist before they become concrete runtime values.
Properties should eventually resolve from static values, keyframes, expressions, bindings, or other explicit derived sources.

### 4. Timeline and animation evaluation
Responsible for answering the question: what is true at frame or time t?
This includes visibility, animated values, easing, interpolation, time remap, frame blending, and evaluation order.

### 5. Expression and binding system
Responsible for deterministic expression evaluation and data-driven property wiring.
Every animatable property should eventually be able to resolve from static values, keyframes, expressions, or data bindings.

### 6. Transform and hierarchy system
Responsible for local transforms, world transforms, anchors, parent-child chains, null-based rigs, and world-space evaluation.

### 7. Camera and optics system
Responsible for camera pose, one-node versus two-node behavior, point of interest logic, projection matrices, focal settings, focus distance, aperture, depth of field inputs, and future optical effects.

### 8. Shape and text systems
Responsible for vector paths, fills, strokes, trim paths, repeaters, text layout, per-glyph animation, and future extrude behavior.

### 9. Effect graph and compositing
Responsible for per-layer effects, masks, mattes, adjustment layers, render passes, precomp passes, depth-aware passes, and final composite assembly.

### 10. Rendering backend
Responsible for actual pixel generation, software rasterization, text draw, shape draw, image draw, blend operations, depth handling, shadow support, and output surfaces.

### 11. Color system
Responsible for working space definitions, linear-light assumptions, transform pipelines, and future OCIO integration.

### 12. Data and template systems
Responsible for external data ingestion, template overrides, master properties, render-job boundaries, and the bridge between reusable scene definitions and runtime parameters.

### 13. Render graph and caching
Responsible for pass planning, cache keys, incremental graph execution, memoization, and deterministic reuse of expensive intermediate results.

### 14. Execution and parallelism model
Responsible for turning evaluated scene state into explicit work that can be scheduled efficiently.
This includes frame-level, pass-level, and other dependency-aware parallel execution opportunities.

### 15. Encoding layer
Responsible for converting frames and audio into final media output.
This layer should remain downstream of rendering.

## Dependency direction

The engine should generally flow in this direction:

spec -> bindings and overrides -> property resolution -> evaluation -> render graph -> renderer -> encoder

The reverse direction should be avoided wherever possible.

## Important architectural rules

### Rule 1
Scene code must not depend on pixel buffer details.

### Rule 2
Camera code must not directly render pixels.
It should produce transforms, matrices, and optical parameters.

### Rule 3
The renderer should consume evaluated state, not raw scene specs.

### Rule 4
Compositing is its own system.
It must not be hidden inside encoding or scene parsing.

### Rule 5
3D behavior must emerge from scene, transform, camera, and light systems.
It must not be bolted on as a visual trick.

### Rule 6
Effects, expressions, and bindings are core systems, not optional extras.
They are part of what makes the engine programmatic.

### Rule 7
Caching must be deterministic and based on explicit inputs.
If a result cannot be keyed and reused safely, it is not ready to become a first-class subsystem.

### Rule 8
Parallel execution must not change output.
Scheduling strategy is an optimization layer, not a semantic layer.

## Phase one intent

The first project phase should establish documentation and boundaries for the major systems.
Implementation comes after the architecture is stable enough to avoid a monolithic design and after the engine identity is clear enough to protect against premature patchwork.