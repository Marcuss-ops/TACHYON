# Background Domain Convergence

## Architecture Debt
The current background system (`BackgroundSpec`) has a separate rendering path in `composition_renderer.cpp`. This forces the renderer to know about `BackgroundSpec` and `BackgroundType`, which violates the principle that the renderer should only deal with normalized layers.

## Goal
Consolidate background rendering into the standard layer pipeline. `BackgroundSpec` should be resolved into one or more `LayerSpec` objects early in the compilation/evaluation process.

## Resolution Plan
1. **Source of Truth**: `BackgroundSpec` remains the authoring-time intent.
2. **Resolution Phase**: During `SceneCompiler::compile` or `SceneSpec` normalization, if a background is present, it is converted into a standard layer at index 0.
   - `BackgroundType::Color` -> `LayerType::Solid`
   - `BackgroundType::Asset` -> `LayerType::Image`
   - `BackgroundType::Component` -> `LayerType::Precomp`
   - `BackgroundType::Preset` -> `LayerType::Procedural`
3. **Renderer**: The renderer will only perform a default clear (usually transparent or black) and then render all layers, including the resolved background layer.

## Implementation Details
- Add a normalization pass to `SceneSpec` or handle it in `SceneCompiler`.
- Update `CompositionBuilder` to handle background-to-layer conversion if possible (authoring-time resolution).
- Remove `BackgroundSpec` handling from `composition_renderer.cpp`.
