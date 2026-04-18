# Architecture Overview

## Architectural goal

TACHYON should be built as a scene and compositing engine, not as a single renderer file and not as a browser replacement glued together from unrelated parts.

The architecture must make room for:

- compositions
- layers
- 2D and 3D transforms
- parenting
- camera systems
- animation evaluation
- render graph construction
- compositing passes
- native encoding

## Major subsystems

### 1. Spec system
Responsible for reading and validating external scene descriptions.
This layer defines the input contract.

### 2. Scene model
Responsible for internal representations of scenes, compositions, nodes, layers, precomps, nulls, and cameras.
This is the conceptual heart of the engine.

### 3. Timeline and animation evaluation
Responsible for answering the question: what is true at frame or time t?
This includes visibility, animated values, easing, interpolation, and evaluation order.

### 4. Transform and hierarchy system
Responsible for local transforms, world transforms, anchors, parent-child chains, and world-space evaluation.

### 5. Camera and optics system
Responsible for camera pose, one-node versus two-node behavior, point of interest logic, projection matrices, focal settings, focus distance, aperture, and future optical effects.

### 6. Render graph and compositing
Responsible for constructing the sequence of passes needed to turn evaluated scene state into a frame.
This may include world passes, overlay passes, precomp passes, depth-aware passes, matte passes, and final composite passes.

### 7. Rendering backend
Responsible for actual pixel generation, software rasterization, text draw, image draw, blend operations, depth handling, and output surfaces.

### 8. Encoding layer
Responsible for converting frames and audio into final media output.
This layer should remain downstream of rendering.

## Dependency direction

The engine should generally flow in this direction:

spec -> scene -> evaluation -> render graph -> renderer -> encoder

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
3D behavior must emerge from scene, transform, and camera systems.
It must not be bolted on as a visual trick.

## Phase one intent

The first project phase should establish documentation and boundaries for the major systems.
Implementation comes after the architecture is stable enough to avoid a monolithic design.
