# Canonical Engine Contract (v1.0.0-LOCKED)

This document represents the "signed" fundamental contract of the TACHYON engine. All subsystems (Compositing, 3D Rendering, Animation, I/O) must adhere to these rules without exception.

## 1. Coordinate System and Handedness

> [!IMPORTANT]
> TACHYON uses a **Right-Handed** coordinate system with **Y-Up** as the global vertical axis.

| Domain | Origin | X-Axis | Y-Axis | Z-Axis | Units |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Raster** | Top-Left | Right | Down | N/A | Integers |
| **Composition** | Top-Left | Right | Down | Layer Stack | Composition Pixels |
| **3D World** | Center (0,0,0) | Right | Up | Backward (-Z is Forward) | Scene Units (Meters) |
| **Camera Local**| Center (0,0,0) | Right | Up | Backward | Focal/Aperture Units |

- **Rotation Order**: Y -> X -> Z (Tait-Bryan, extrinsic).
- **Matrix Convention**: Column-Major (OpenGL/Vulkan style).
- **Camera Forward**: Points toward **-Z**.

## 2. Fundamental Units

- **Distance**: Internal units are interpreted as **Meters** for 3D and **Pixels** for 2D. 1px = 1 unit unless scaled.
- **Time**: Internal time is **Seconds (double)**. Any frame-based logic must be computed using `Math.floor(time * fps + epsilon)`.
- **Angles**: Internal calculations use **Radians**. Authored Scene Spec uses **Degrees**.
- **Focal Length**: Millimeters (mm).
- **Aperture**: F-stop (dimensionless).

## 3. Alpha Model

> [!CAUTION]
> TACHYON is a **Premultiplied Alpha** engine.

- **Internal Storage**: All textures/buffers in the render graph are stored as (R*A, G*A, B*A, A).
- **Handoff**: Assets (PNG/EXR) are converted to Premultiplied during the **Decode** stage.
- **Blending**: Standard `SrcOver` is `Color_dest * (1 - Alpha_src) + Color_src`.
- **Masks/Mattes**: Applied purely to the Alpha channel; Color channels are updated to maintain the premultiplied invariant.

## 4. Working Color Model

- **Precision**: 32-bit Float (FP32) per channel for all compositing and 3D paths.
- **Working Space**: **Linear-Light**.
- **Primaries**: Rec.709 / sRGB (Linear).
- **Transfer Function**: Output transforms (e.g., sRGB Gamma 2.2) are applied ONLY at the final Encode/Display stage.
- **OCIO**: Architecture must allow for OCIO `Config` injection in v2, but v1 is fixed to Linear Rec.709.

## 5. Time Semantics

- **Evaluation Time**: Every layer evaluates at a single `time` value passed down from the Root Composition.
- **Local Time**: `LocalTime = (CompTime - StartTime) * TimeStretch + TimeRemap(CompTime)`.
- **Subframe Sampling**: The engine supports arbitrary subframe evaluation for Motion Blur. No "frame snapping" occurs unless explicitly requested by a "Hold Frame" policy.
- **Precompositions**: Evaluate as isolated graph branches with their own time domain.

## 6. Deterministic Seed Policy

- **Root Seed**: Every Render Job has a mandatory 64-bit `seed`.
- **Hashed Seeds**: Feature seeds are generated via `MurmurHash3(RootSeed ^ ScopeID ^ FrameIndex)`.
- **ScopeID**: A stable string or integer representing the layer/effect/property path.

## 7. Surface and AOV Naming

Standard names for v1:
- `beauty`: Normalized RGB color.
- `alpha`: Coverage/Transparency.
- `depth`: Z-Distance from camera plane (Positive = Forward distance, Linear).
- `normal`: World-space surface normal [X, Y, Z] encoded as [-1, 1].
- `albedo`: Base diffuse/emissive color without lighting.

## 8. Diagnostics and Errors

- **Structured Logs**: Every log must include a `Code` (e.g., `TY-1002`) and a `SourcePath` (Scene Spec JSON path).
- **Recoverable**: Warnings that don't stop the render (e.g., missing optional asset, use fallback).
- **Fatal**: Errors that terminate the job (e.g., invalid schema, division by zero in expression, resource exhaustion).

## 9. Compatibility and Versioning

- **Scene Spec Versioning**: Semantic Versioning (SemVer). 
- **Major**: Breaking structural changes to the specification.
- **Minor**: New effects or properties that fallback safely.
- **Patch**: Bug fixes in schema validation.
- **Consistency**: The same Version + Seed + Job Spec MUST produce pixel-identical results.
