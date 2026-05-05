# 3D Rendering

Documentation for the 3D subsystem, including cameras, lights, scene bridging, and render backends.

## Topics

- Camera models
- Lights
- Backend strategy
- 2D/3D boundaries
- Path tracing

## Current Model

The 3D stack stays separate from the 2D compositing path:

- 2D layers are evaluated in the scene pipeline first
- 3D data is bridged through the dedicated 3D scene structures
- the renderer3d backend owns 3D-specific rendering behavior

## How to Add a 3D Feature

1. Extend the 3D spec or authoring surface if the feature needs new inputs.
2. Add evaluator or bridge support so the runtime sees the new data.
3. Implement the renderer backend change in the existing 3D path.
4. Keep 2D rendering untouched unless the feature truly crosses the boundary.
5. Add a focused 3D smoke or integration test.

## Rules

- Do not route 3D through a separate one-off renderer.
- Do not mix 2D compositing semantics into the 3D backend.
- Do not add hidden defaults that change scene behavior.
- Do not replace the existing bridge model with a parallel shortcut.
