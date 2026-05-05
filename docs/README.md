# Tachyon Documentation

Welcome to the Tachyon engine documentation.

## Quick Navigation

- [Project Vision](00-project/vision.md) - What Tachyon is and isn't
- [Non-Goals](00-project/non-goals.md) - What we explicitly don't do
- [MVP v1](00-project/mvp-v1.md) - First milestone scope
- [Architecture](10-architecture/headless-core.md) - Core architecture overview
- [Backgrounds](30-scene-and-animation/backgrounds.md) - Background authoring and registries
- [Transitions](30-scene-and-animation/transitions.md) - Transition authoring and registries
- [Text Module](../src/text/README.md) - Text subsystem overview
- [2D Compositing](40-2d-compositing/README.md) - Layer compositing pipeline
- [3D Rendering](50-3d/README.md) - 3D subsystem overview
- [Golden Tests](90-testing/golden-tests.md) - Visual regression testing

## Documentation Structure

- `docs/00-project/` — vision, non-goals, roadmap, MVP scope
- `docs/10-architecture/` — architecture, execution model, determinism, parallelism, scene foundations
- `docs/20-contracts/` — cross-cutting engine contracts
- `docs/30-scene-and-animation/` — scene spec, layers, properties, animation
- `docs/40-2d-compositing/` — shapes, masks, effects, text, motion blur
- `docs/50-3d/` — cameras, lights, backend strategy, path tracing
- `docs/60-runtime/` — caching, memory policy, quality tiers, profiling
- `docs/70-media-io/` — asset pipeline, decode/encode, output delivery
- `docs/80-audio/` — audio reactivity and audio-driven animation
- `docs/90-testing/` — canonical scenes and validation

## For Contributors

Before changing code, read:
1. `TACHYON_ENGINEERING_RULES.md`
2. The relevant contract and subsystem folders for the area being worked on
