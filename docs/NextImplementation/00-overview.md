# Tachyon Next Implementation - Overview

> **Purpose:** This document tracks implementation progress for Tachyon motion-design and VFX workstation.

## Core Principles
- Keep core engine semantics **deterministic**
- Keep tracking, masking, timing, and camera behavior **first-class** in scene model
- Keep AI and editor conveniences as **optional accelerators**, not hidden dependencies

## Codebase State Summary

| System | Status | Location |
|--------|--------|----------|
| Feature Tracker (Harris + LK optical flow) | ✅ Implemented | `include/tachyon/tracker/feature_tracker.h` |
| Physical Camera Model | ✅ Implemented | `include/tachyon/core/camera/camera_state.h` |
| Two-Node Camera / POI | ✅ Implemented | `CameraState::target_position`, `use_target` |
| Camera Rigging / Parenting | ✅ Implemented | `include/tachyon/core/scene/rigging/rig_graph.h` |
| Text Animator Pipeline | ✅ Implemented | `include/tachyon/text/animation/text_animator_pipeline.h` |
| Mask Renderer | ✅ Implemented | `include/tachyon/renderer2d/evaluated_composition/mask_renderer.h` |
| Environment Manager (PBR, IBL) | ✅ Implemented | `include/tachyon/renderer3d/lighting/environment_manager.h` |
| Motion Blur (contract) | ⚠️ Partial | `include/tachyon/renderer3d/effects/motion_blur.h` |
| Bezier Interpolation | ✅ Implemented | `include/tachyon/core/properties/bezier_interpolator.h` |
| Camera Shake | ⚠️ Partial | `include/tachyon/core/camera/camera_shake.h` |
| Disk Cache / Cache Key | ⚠️ Partial | `runtime/cache/`, `media/cache/` |
| AI Segmentation Provider | ✅ Contract only | `ai/segmentation_provider.h` |
| Ray Tracer (Embree) | ✅ Implemented | `renderer3d/core/ray_tracer.cpp` |
| Depth Z AOV | ✅ Implemented | Produced by `ray_tracer.cpp` |
| Scene Contracts / Serialization | ❌ Missing | `docs/NextImplementation/scene_contracts.md` |

## Progress Estimate
- **~35% completed** against full plan
- Foundations (camera, tracker, text, ray tracer) are in place
- Missing: shared scene contracts, pipeline layers that make features usable (bindings, DoF, matte semantics), and editor-facing inspection

## File Structure
```
docs/NextImplementation/
├── 00-overview.md              # This file
├── roto_masking.md             # Roto, Masks, Matte Semantics
├── scene_contracts.md          # Shared scene schema and reference contracts
├── camera_3d.md                # Camera & 3D Rendering
├── temporal_effects.md         # Temporal Effects (Time Remap, etc.)
├── interpolation_graph.md      # Interpolation & Graph Editor
├── text_animator_pro.md        # Text Animator Pro
└── editor_support.md           # Editor-facing Support
```

## Recommended Implementation Order
0. **Scene contracts** — shared IDs, references, schema versioning, and validation
1. **Lock contracts** — define headers and data models
2. **Tracking pipeline** — point tracker binding, planar tracker, camera solver
3. **Masks & matte** — variable feather, track matte semantics
4. **Cache & proxy** — disk cache + RAM preview with diagnostics
5. **Camera features** — DoF, camera shake, camera cuts
6. **Temporal pipeline** — time remap, frame blending, motion blur
7. **Animation curves** — graph editor backend + text on path
8. **AI layers** — roto brush, rolling shutter on top of stable primitives
