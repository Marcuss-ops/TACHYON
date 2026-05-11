# 3D Renderer Module

This module owns the 3D scene bridge, materials, animation helpers, and the backend factory used by the 2D runtime.

## Directory Structure

```
src/renderer3d/
├── core/              # Core 3D plumbing and backend factory
├── animation/         # 3D animation
├── materials/         # Material system
├── visibility/        # Visibility determination
├── temporal/          # Time-based effects
├── effects/           # 3D effects
└── utilities/         # Utilities
```

## Key Components

- `renderer3d_backend_factory`: backend seam for the 3D renderer
- Materials: PBR material system and evaluators
- Animation: motion blur and temporal helpers
- Effects: depth of field and related utilities

## Notes

- The active backend is selected outside this module.
- The current tree keeps the 3D pipeline buildable while the concrete backend is being replaced.
