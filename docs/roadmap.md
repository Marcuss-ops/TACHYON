# Roadmap

## Phase 0 — Foundations

Establish the project identity and architecture before writing engine code.
This phase is documentation-heavy by design.

Deliverables:

- vision
- non-goals
- architecture overview
- expression system design
- effects system design
- masking and matte design
- shape system design
- text animator design
- light system design
- time system design
- template system design
- color management design
- deterministic caching design
- audio reactivity design
- data binding design

## Phase 1 — Core scene engine

Focus on the minimal architectural backbone required for a real scene engine.

Target systems:

- math primitives
- transforms
- hierarchy and parenting
- core scene model
- one-node and two-node camera support
- point of interest support
- basic timeline evaluation

## Phase 2 — Motion graphics core

Turn the scene engine into a usable motion engine.

Target systems:

- shape layers
- masks and track mattes
- text layout foundation
- text animator framework
- first effect graph path
- initial compositing pipeline

## Phase 3 — Programmatic engine

Make the engine truly data-driven and reusable.

Target systems:

- expression evaluation
- data binding
- published template properties
- external override flows
- audio-driven property hooks

## Phase 4 — Spatial and optical growth

Expand the camera and spatial rendering model.

Target systems:

- lighting model
- depth-aware compositing
- early DOF pipeline inputs
- stronger 2.5D and 3D workflows

## Phase 5 — Performance and scale

Focus on deterministic reuse and render efficiency.

Target systems:

- cache keys
- incremental render graph work
- memoization strategy
- profiling and optimization loops

## Guiding rule

The project should grow in system depth before feature breadth.
If a feature does not strengthen the engine model, it should not jump ahead of foundational work.
