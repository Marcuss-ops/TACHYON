# Time System

## Purpose

A motion graphics engine must treat time as a controllable domain, not just a monotonically advancing frame counter.
This is necessary for asset reuse, retiming, template systems, and advanced compositing workflows.

## Planned capabilities

- time remapping
- time stretch
- hold frames
- frame blending
- future motion-vector-aware workflows

## Architectural role

The time system belongs to scene evaluation and timeline control.
It should influence how layers, precomps, and asset-backed sources are sampled over time.

## Planned structure

```text
src/timeline/
├── time_remap.cpp
├── frame_blend.cpp
└── time_stretch.cpp
```

## Design rules

1. Retiming must remain deterministic.
2. Time remap must integrate with caching and dependency tracking.
3. Precompositions must be able to evaluate under altered local time domains.
4. Frame blending belongs to the timeline or render planning model, not as a random effect bolted on later.
