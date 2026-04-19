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
- determinism design
- parallelism design
- render job model
- scene spec design
- render strategy design
- pixel pipeline design
- camera and 3D scene design
- lighting and materials design
- 3D mesh and extrusion design
- expression system design
- effects system design
- masking and matte design
- shape system design
- text animator design
- text layout and shaping design
- time system design
- motion blur design
- color management design
- asset pipeline design
- decode and encode design
- voice-over and subtitles design
- multi-format output design
- deterministic caching design
- audio reactivity design
- data binding design
- testing and compatibility design

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
- first compositing path
- encoded output through a native media pipeline

## Phase 2 — Motion graphics and text core

Turn the scene engine into a usable motion graphics engine.

Target systems:

- shape layers
- masks and track mattes
- text layout foundation
- text animator framework
- subtitle burn-in path
- first effect graph path
- initial multi-format output support

## Phase 3 — Programmatic and data-driven engine

Make the engine truly reusable, automatable, and media-pipeline friendly.

Target systems:

- expression evaluation
- data binding
- scene parameter overrides
- render-job override flows
- audio-driven property hooks
- voice-over timing hooks
- subtitle timing and forced-alignment integration points

## Phase 4 — 3D scene and offline rendering

Expand the engine into a serious CPU-first 3D rendering system.

Target systems:

- perspective and orthographic cameras
- lights and materials
- mesh import
- text and shape extrusion
- hybrid 2D and 3D render planning
- Embree-backed ray intersection path
- early path-traced lighting, reflections, refractions, and DOF

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

## Guiding rule

The project should grow in system depth before feature breadth.
If a feature does not strengthen the engine model, it should not jump ahead of foundational work.
