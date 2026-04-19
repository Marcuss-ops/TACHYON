# Effects System

## Purpose

The effects system allows layers and compositions to be processed through deterministic image operations without hardcoding special-case rendering paths for each visual treatment.

## Core model

Each renderable layer may own an ordered list of effect instances.
Each effect instance should define:

- a type
- animatable parameters
- required inputs
- optional auxiliary inputs
- output characteristics
- region of interest behavior
- whether it requires depth or world information

## Architectural role

The effects system sits after scene evaluation and before or during compositing, depending on the effect category.
It does not replace the renderer. It extends the image-processing pipeline around it.

## Planned structure

```text
src/effects/
├── effect_graph.cpp
├── registry.cpp
├── effect_instance.cpp
├── effect_host.cpp
└── builtins/
```

## Initial built-in priorities

- gaussian blur
- glow
- drop shadow
- levels or curves
- hue saturation
- fill
- tint
- fractal noise
- displacement map
- gradient ramp
- feather
- stroke
- camera lens blur
- chromatic aberration
- vignette
- film grain
- LUT application

## Design rules

1. Effects must be deterministic.
2. Effect parameters must be animatable like any other property.
3. The system must support future plugin-style extension.
4. Effects should declare enough metadata to participate in render planning and caching.
5. Adjustment-style workflows must remain possible at the compositing level.
