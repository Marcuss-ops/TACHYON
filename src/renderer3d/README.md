# 3D Renderer Module

3D rendering pipeline with ray tracing, materials, and animation.

## Directory Structure

```
src/renderer3d/
├── core/              # Core 3D rendering
│   ├── ray_tracer.cpp       # Main ray tracer
│   ├── ray_tracer_shading.cpp # Shading model
│   ├── ray_tracer_scene.cpp  # Scene management
│   └── scene_cache.cpp       # Scene caching
├── animation/         # 3D animation
│   ├── motion_blur.cpp
│   └── animation3d.cpp
├── materials/         # Material system
│   ├── material.cpp
│   ├── material_evaluator.cpp
│   └── pbr_utils.cpp
├── visibility/        # Visibility determination
│   └── culling.cpp
├── temporal/          # Time-based effects
│   ├── time_remap_curve.cpp
│   ├── frame_blend_mode.cpp
│   └── rolling_shutter.cpp
├── effects/           # 3D effects
│   └── depth_of_field.cpp
└── utilities/         # Utilities
    ├── debug_tools.cpp
    └── environment_manager.cpp
```

## Key Components

- **Ray Tracer**: CPU-based ray tracing with Embree integration
- **Materials**: PBR material system with evaluators
- **Animation**: 3D animation with motion blur
- **Effects**: Depth of field, temporal effects

## Dependencies
- Embree: Intel's ray tracing library
- Optional: Vulkan for GPU acceleration
