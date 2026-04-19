# Documentation Index

This index is the starting point for navigating TACHYON's architecture and system design documents.

The repository currently contains both older flat documents under `docs/` and a newer grouped structure under subfolders.
This mixed structure is intentional during the transition.

## How to read the docs

Recommended reading order for a new contributor:

1. `vision.md`
2. `non-goals.md`
3. `architecture.md`
4. `roadmap.md`
5. `mvp-v1.md`
6. grouped system contracts below

## Grouped documentation

### Contracts
- `contracts/core-contracts.md`
- `contracts/rendering-contract.md`

### Scene and compositing
- `scene/composition-layer-model.md`
- `scene/scene-model.md`
- `scene/scene-spec.md`
- `scene/scene-spec-v1.md`
- `scene/camera-system.md`
- `scene/camera-and-3d-scene.md`
- `scene/light-system.md`
- `scene/lighting-and-materials.md`
- `scene/masking.md`
- `scene/compositing.md`
- `scene/2d-3d-compositing-boundary.md`
- `scene/shape-system.md`
- `scene/text-animator.md`
- `scene/text-layout-and-shaping.md`
- `scene/3d-mesh-and-extrusion.md`

### Render and pixel pipeline
- `render/surface-and-aov-contract.md`
- `render/render-strategy.md`
- `render/render-backend-strategy.md`
- `render/pixel-pipeline.md`
- `render/path-tracing-cpu.md`
- `render/motion-blur.md`
- `render/color-management.md`

### Runtime and execution policy
- `runtime/memory-and-resource-policy.md`
- `runtime/quality-tiers.md`
- `runtime/execution-model.md`
- `runtime/property-model.md`
- `runtime/property-animation-system.md`
- `runtime/animation-system.md`
- `runtime/expression-system.md`
- `runtime/expression-runtime.md`
- `runtime/time-system.md`
- `runtime/parallelism.md`
- `runtime/caching.md`
- `runtime/dependency-graph-and-invalidation.md`
- `runtime/determinism.md`
- `runtime/audio-reactivity.md`
- `runtime/audio-driven-animation.md`
- `runtime/effect-registry.md`
- `runtime/effects.md`
- `runtime/effects-priority-matrix.md`
- `runtime/performance-tiers.md`
- `runtime/packaging-runtime-layout.md`

### Output and delivery
- `output/output-profile-schema.md`
- `output/encoder-output.md`
- `output/decode-encode.md`
- `output/render-job.md`
- `output/multi-format-output.md`
- `output/asset-pipeline.md`
- `output/voice-over-and-subtitles.md`

### Interfaces
- `interfaces/cli-and-api-contract.md`
- `interfaces/api.md`
- `interfaces/cli.md`

### Observability
- `observability/diagnostics-and-profiling.md`
- `observability/error-handling.md`

### Testing
- `testing/canonical-scenes.md`
- `testing/testing-and-compatibility.md`

## Migration note

The reorganization phase is now complete. All internal documentation has been moved to its respective category in `docs/`.
New documents should be added to a subfolder unless they are broad top-level project docs.

## Folder intent

### `docs/contracts/`
Cross-cutting rules the whole engine must obey.

### `docs/scene/`
Scene model, layers, cameras, lights, shapes, text, and compositing hierarchy.

### `docs/render/`
Render surfaces, pass semantics, compositing handoffs, and backend-facing rendering contracts.

### `docs/runtime/`
Execution, memory, quality, fallback, and operational behavior.

### `docs/output/`
Render-job output profiles, delivery targets, and encoding-adjacent schemas.

### `docs/interfaces/`
CLI and service-facing contracts.

### `docs/observability/`
Logs, metrics, profiling, debugging, and diagnostics.

### `docs/testing/`
Golden scenes, validation packs, and compatibility-oriented checks.

