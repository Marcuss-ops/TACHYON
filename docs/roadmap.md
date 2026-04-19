# Roadmap

## Phase 0 — Foundations

Establish the project identity and architecture before writing engine code.
This phase is documentation-heavy by design.

Deliverables:

- vision
- non-goals
- architecture overview
- execution model
- property model
- property animation model
- animation system design
- determinism design
- parallelism design
- render job model
- scene model design
- scene spec design
- render backend strategy
- 2D and 3D compositing boundary
- expression system design
- expression runtime design
- effects system design
- effects priority matrix
- masking and matte design
- shape system design
- text animator design
- text layout and shaping design
- camera system design
- light system design
- time system design
- color management design
- asset pipeline design
- deterministic caching design
- audio reactivity design
- audio-driven animation design
- data binding design
- testing and compatibility design
- CPU path tracing direction

## Phase 1 — Core 2D scene and compositing engine

Focus on the minimal architectural backbone required for a real headless motion engine.

Target systems:

- math primitives
- transforms
- hierarchy and parenting
- core composition and layer model
- scene spec parsing and validation
- basic timeline evaluation
- initial property model integration
- image and solid layer rendering
- first text path
- first shape path
- first compositing path
- encoded output through a native media pipeline

## Phase 2 — Motion graphics and text core

Turn the scene engine into a usable motion graphics engine.

Target systems:

- masks and track mattes
- serious text layout foundation
- text animator framework
- subtitle burn-in path
- first effect host path
- first effect set from the priority matrix
- adjustment-layer semantics
- initial multi-format output support

Initial feature cut for this phase:

- solid, shape, text, image, video, audio, null, camera, light, mesh 3D, precomp, adjustment layers
- shape paths with bezier curves, stroke, fill, trim paths, repeater, and offset path
- text animators for position, rotation, scale, opacity, tracking, and fill
- per-word or per-character selectors, including range and expression selectors
- a compact effect set centered on blur, glow, shadow, color correction, distortion, grain, and LUT application

## Phase 3 — Programmatic and data-driven engine

Make the engine truly reusable, automatable, and media-pipeline friendly.

Target systems:

- expression evaluation
- bounded expression runtime
- data binding
- scene parameter overrides
- render-job override flows
- audio feature ingestion
- audio-driven property hooks
- voice-over timing hooks
- subtitle timing and forced-alignment integration points

## Phase 4 — 3D scene and offline rendering

Expand the engine into a serious CPU-first 3D rendering system without collapsing the compositor into a monolithic renderer.

Target systems:

- perspective cameras with explicit lens and DOF semantics
- lights and materials
- glTF-first mesh import
- render-pass contract for 3D layers and precomps
- Embree-backed ray intersection path
- Open Image Denoise integration as an explicit post-render stage
- early path-traced lighting, reflections, refractions, shadows, and DOF
- text and shape extrusion after the scene and text foundations are stable

## Phase 5 — Performance and scale

Focus on deterministic reuse and render efficiency.

Target systems:

- cache keys
- incremental render graph work
- memoization strategy
- profiling and optimization loops
- dependency-aware parallel execution
- ROI and tiling strategy
- memory budget enforcement
- render quality tiers for weak CPU environments

## Execution backlog

The project still needs these concrete runtime blocks before it feels like a real headless motion engine:

- scene graph runtime
- serious 2D compositing
- media pipeline
- programmatic interface
- CPU offline 3D
- performance on weak machines

The detailed gap list lives in `docs/implementation-gaps.md`.

## Guiding rule

The project should grow in system depth before feature breadth.
If a feature does not strengthen the engine model, it should not jump ahead of foundational work.
