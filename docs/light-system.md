# Light System

## Purpose

Lighting turns 3D and 2.5D scene support from simple parallax into a more complete spatial rendering model.
Even an initial light system creates room for shading, shadow logic, and richer depth cues.

## Initial light types

- point light
- spot light
- ambient light

## Architectural role

Lights are scene nodes, not renderer toggles.
They participate in hierarchy, animation, scene evaluation, and render planning.

## Planned structure

```text
src/light/
├── light_layer.cpp
├── point_light.cpp
├── spot_light.cpp
└── ambient_light.cpp

src/render/
└── shadow_map.cpp
```

## Design rules

1. Lights must be animatable and deterministic.
2. Light evaluation must occur before affected shading passes are executed.
3. The architecture should allow future shadows and material interaction without rewriting the scene model.
4. Lights should remain optional for simple workflows but first-class in the engine model.
