# Tachyon Rendering Pipeline: Stabilization & Hardening Complete

I have resolved the critical I/O blockers and incomplete expression features identified in the review. Tachyon is now capable of producing valid video files and executing complex AE-parity expressions.

## Changes Overview

### 1. Video Encoder Stabilization
- **Fixed I/O Mismatch**: Modified `ffmpeg_video_encoder.cpp` to perform manual float-to-uint8 conversion for every pixel.
- **Color Correction**: Implemented clamping to [0, 255] to prevent overflows when rendering high-intensity (HDR) frames to standard formats.

### 2. Expression Engine Hardening
- **`loopOut()` Implementation**: Added support for duration-based looping in `expression_engine.cpp`.
- **Context Injection**: Updated `sample_scalar` to pass `layer_index` and a recursive `PropertySampler`.
- **Recursion Safety**: Added a `skip_expression` flag to allow remapping functions to sample the "base" keyframe values without triggering infinite expression loops.
- **`stagger()` Support**: Fully wired `layer_index` into the context, enabling procedural delays based on layer stack position.

## Technical Details

### Float to Byte Conversion
```cpp
// src/output/ffmpeg_video_encoder.cpp
rgba[0] = static_cast<std::uint8_t>(std::clamp(color.r * 255.0f, 0.0f, 255.0f));
// ... repeated for G, B, A
```

### Expression Context Wiring
```cpp
// src/core/scene/evaluator_utils.cpp
if (!property.keyframes.empty()) {
    expr_ctx.variables["_prop_start"] = property.keyframes.front().time;
    expr_ctx.variables["_prop_end"] = property.keyframes.back().time;
    expr_ctx.variables["_prop_duration"] = property.keyframes.back().time - property.keyframes.front().time;
}
```

## Verification Results

- [x] **Video Integrity**: Output video files now open correctly in standard players (VLC, QuickTime).
- [x] **Looping behavior**: `loopOut()` accurately repeats keyframe cycles beyond the last keyframe.
- [x] **Stagger**: Verified that `stagger(delay)` correctly offsets layer animations.

> [!NOTE]
> The engine is now ready for deployment to the staging environment for visual parity testing against Adobe After Effects.
