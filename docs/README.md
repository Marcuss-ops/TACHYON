# TACHYON Documentation Index

Welcome to the TACHYON documentation. This repository uses a numbered architectural structure to organize design goals, contracts, and system details.

## Core Project & Vision
- **00 Project**
  - [Project Vision](00-project/vision.md)
  - [Roadmap](00-project/roadmap.md)
  - [MVP V1 Goals](00-project/mvp-v1.md)
  - [Non-Goals](00-project/non-goals.md)

- **10 Architecture**
  - [Core Architecture](10-architecture/architecture.md)
  - [Execution Model](10-architecture/execution-model.md)
  - [Determinism Policy](10-architecture/determinism.md)
  - [Parallelism Strategy](10-architecture/parallelism.md)
  - [Dependency Graph & Invalidation](10-architecture/dependency-graph-and-invalidation.md)
  - [Scene Model Design](10-architecture/scene-model.md)
  - [Pixel Pipeline](10-architecture/pixel-pipeline.md)
  - [Error Handling](10-architecture/error-handling.md)

## System Contracts
- **20 Contracts**
  - [Core Contracts](20-contracts/core-contracts.md)
  - [Rendering Contract](20-contracts/rendering-contract.md)
  - [Surface & AOV Contract](20-contracts/surface-and-aov-contract.md)
  - [Output Profile Schema](20-contracts/output-profile-schema.md)
  - [CLI & API Contract](20-contracts/cli-and-api-contract.md)
  - [Testing & Compatibility](20-contracts/testing-and-compatibility.md)
  - [API Reference](20-contracts/api.md)
  - [CLI Reference](20-contracts/cli.md)

## Systems & Subsystems
- **30 Scene & Animation**
  - [Scene Specification](30-scene-and-animation/scene-spec.md)
  - [Scene Spec V1](30-scene-and-animation/scene-spec-v1.md)
  - [Composition Layer Model](30-scene-and-animation/composition-layer-model.md)
  - [Property Model](30-scene-and-animation/property-model.md)
  - [Property Animation System](30-scene-and-animation/property-animation-system.md)
  - [Animation System](30-scene-and-animation/animation-system.md)
  - [Time System](30-scene-and-animation/time-system.md)
  - [Template System](30-scene-and-animation/template-system.md)
  - [Data Binding](30-scene-and-animation/data-binding.md)
  - [Expression System](30-scene-and-animation/expression-system.md)
  - [Expression Runtime](30-scene-and-animation/expression-runtime.md)
  - [Remotion-Style Authoring Map](30-scene-and-animation/remotion-style-authoring-map.md)
  - [3D Mesh & Extrusion Placeholder](30-scene-and-animation/3d-mesh-and-extrusion.md)

- **40 2D Compositing**
  - [Shape System](40-2d-compositing/shape-system.md)
  - [Masking & Mattes](40-2d-compositing/masking.md)
  - [Effects Pipeline](40-2d-compositing/effects.md)
  - [Effects Priority Matrix](40-2d-compositing/effects-priority-matrix.md)
  - [Effect Registry](40-2d-compositing/effect-registry.md)
  - [Text Layout & Shaping](40-2d-compositing/text-layout-and-shaping.md)
  - [Text Animator](40-2d-compositing/text-animator.md)
  - [Motion Blur](40-2d-compositing/motion-blur.md)
  - [Color Management](40-2d-compositing/color-management.md)
  - [Compositing Logic](40-2d-compositing/compositing.md)

- **50 3D**
  - [Camera System](50-3d/camera-system.md)
  - [Camera & 3D Scene Interactions](50-3d/camera-and-3d-scene.md)
  - [Light System](50-3d/light-system.md)
  - [Lighting & Materials](50-3d/lighting-and-materials.md)
  - [2D/3D Boundary](50-3d/2d-3d-compositing-boundary.md)
  - [Render Backend Strategy](50-3d/render-backend-strategy.md)
  - [Render Strategy](50-3d/render-strategy.md)
  - [Path Tracing (CPU)](50-3d/path-tracing-cpu.md)

## Lifecycle & Resources
- **60 Runtime**
  - [Scaling Strategy (Caching)](60-runtime/caching.md)
  - [Memory & Resource Policy](60-runtime/memory-and-resource-policy.md)
  - [Quality Tiers](60-runtime/quality-tiers.md)
  - [Performance Tiers](60-runtime/performance-tiers.md)
  - [Low-End Hardware Strategy](60-runtime/low-end-strategy.md)
  - [Diagnostics & Profiling](60-runtime/diagnostics-and-profiling.md)
  - [Packaging & Runtime Layout](60-runtime/packaging-runtime-layout.md)

- **70 Media IO**
  - [Asset Pipeline](70-media-io/asset-pipeline.md)
  - [Decode/Encode Strategy](70-media-io/decode-encode.md)
  - [Encoder Output Contracts](70-media-io/encoder-output.md)
  - [Render Job Logic](70-media-io/render-job.md)
  - [Multi-Format Output](70-media-io/multi-format-output.md)
  - [Voice-Over & Subtitles](70-media-io/voice-over-and-subtitles.md)

- **80 Audio**
  - [Audio Reactivity](80-audio/audio-reactivity.md)
  - [Audio-Driven Animation](80-audio/audio-driven-animation.md)

## Validation
- **90 Testing**
  - [Canonical Scenes](90-testing/canonical-scenes.md)
