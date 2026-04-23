# Temporal Effects

> **Why this matters:** Modern motion design is not just about frames — it's about *time feel*.
> CapCut wins users with fluid speed ramps. AE's Time Remapping with optical flow is the standard.
> Without time remapping + frame blending, 24fps footage looks choppy and amateur.

## Current State

Tachyon needs stronger time-domain behavior for modern motion design.

**What exists:**
- ✅ `src/tracker/feature_tracker.cpp` (optical flow foundation)
- ⚠️ `src/renderer3d/effects/motion_blur.h` (contract only)
- ⚠️ Motion blur sub-frame sampling (partial, see `camera_3d.md`)

**What is missing:**
- Optical-flow time remapping
- Frame blending / pixel motion
- Motion blur tied to same temporal sampling model
- Clear distinction between retiming and blur
- Rolling shutter correction (depends on the tracking pipeline)
- Cache identity for temporal modes and shutter settings

---

## 1. Optical Flow Time Remap

**Status:** ❌ Not implemented

### Use Cases
- Speed ramps (slow → fast → slow)
- Frame rate conversion (24fps → 60fps)
- Synthesize intermediate frames for smooth motion

### Implementation Rules
- Evaluate source motion between adjacent frames
- Warp image through motion field
- Blend or synthesize intermediate frames deterministically
- Fall back to simple hold or blend when flow confidence is poor
- Keep retime and blur in separate contracts
- Make confidence and fallback mode visible to editor and diagnostics

> **Important:** Do NOT make optical flow the only retiming path.
> Engine should still support clean time remap without AI or flow when not needed.

### Files to Create
```
src/timeline/time_remap.cpp              # NEW: Time remap evaluation
src/timeline/time_remap.h
src/timeline/frame_blend.cpp             # NEW: Frame blending modes
src/timeline/frame_blend.h
src/tracker/optical_flow_warp.cpp        # NEW: Flow-based warping
docs/NextImplementation/scene_contracts.md # Shared timing and mode fields
```

### Data Model (Planned)
See `include/tachyon/core/spec/schema/contracts/shared_contracts.h` for `TimeRemapCurve` and `TimeRemapMode`.

```cpp
// Optical flow assisted synthesis
std::vector<float> synthesize_frame_optical_flow(
    const std::vector<float>& frame_a,
    const std::vector<float>& frame_b,
    const std::vector<std::pair<float, float>>& flow_ab, // (dx, dy) per pixel
    float t); // interpolation factor [0, 1]
```

---

## 2. Frame Blending & Pixel Motion

**Status:** ❌ Not implemented

### Implementation Rules
- Support hold, blend, and motion-assisted modes
- Keep selected method explicit in timeline model
- Include it in cache identity (different blend mode = different cache key)

### Integration with Cache
- Add `TimeRemapMode` and `FrameBlendMode` to `CacheKey` (see the runtime cache layer)
- Add motion blur shutter angle, sample count, and proxy mode to the same cache identity

### Data Model (Planned)
See `include/tachyon/core/spec/schema/contracts/shared_contracts.h` for `FrameBlendMode` and `FrameBlendParams`.

---

## 3. Motion Blur (Temporal Sampling)

**Status:** ⚠️ Partial implementation (see `camera_3d.md` Phase 2)

### Implementation Rules
- Sample scene across shutter interval
- Accumulate resulting frames deterministically
- Respect camera, layer, and object motion equally
- Keep sample count and shutter policy visible in render contract

### Relationship to Time Remap
- Motion blur samples **within** a frame's shutter window
- Time remap changes **which source frames** are used
- Both use optical flow but for different purposes
- Motion blur must not consume the time-remap fallback path

### Files (See `camera_3d.md`)
```
src/renderer3d/animation/motion_blur.cpp  # Complete sub-frame sampling
src/renderer3d/effects/motion_blur.h       # Complete header
```

### Data Model (Existing)
See `include/tachyon/core/spec/schema/contracts/shared_contracts.h` for `MotionBlurParams`.

```cpp
std::vector<float> generate_subframe_times(float frame_time, float frame_duration, const MotionBlurParams& params);
```

---

## 4. Rolling Shutter Correction

**Status:** ❌ Not implemented (depends on the tracking pipeline)

### Relationship to Temporal Effects
- Rolling shutter is a **separate temporal distortion model**
- Model scanline timing explicitly
- Apply correction or simulation as dedicated pass
- Keep independent from motion blur exposure logic

### Files (Tracking pipeline)
```
src/tracker/rolling_shutter.cpp
src/tracker/rolling_shutter.h
```

### Required Shared Fields
- Readout time
- Scan direction
- Sensor model or metadata source
- Per-shot enable/disable flag

---

## Implementation Priority
1. ⬜ Implement TimeRemapCurve evaluation (hold, blend modes)
2. ⬜ Implement optical flow synthesis (`synthesize_frame_optical_flow`)
3. ⬜ Implement frame blending modes (linear, pixel motion)
4. ⬜ Complete Motion Blur sub-frame sampling (see `camera_3d.md`)
5. ⬜ Integrate time remap + frame blend into cache key
6. ⬜ Rolling shutter correction (depends on tracking pipeline)

---

## Dependencies
- `FeatureTracker` (for optical flow vectors)
- `CacheKeyBuilder` (add time remap params to cache identity)
- `FrameBuffer` and rasterization pipeline
- Motion blur sub-frame sampling (see `camera_3d.md`)
- `scene_contracts.md` for time-domain modes

---

## Integration Points

### With Cache (runtime cache layer)
- Time remap mode and frame blend mode must be part of `CacheKey`

### With Tracking (tracking pipeline)
- Optical flow from `FeatureTracker` used for:
  - Frame synthesis (this doc)
  - Rolling shutter correction (tracking doc)

### With Camera (`camera_3d.md`)
- Motion blur sub-frame sampling tied to shutter angle
- Camera motion + object motion both contribute to blur

---

## References
- `camera_3d.md` — Motion Blur implementation (Phase 2)
- Tracking pipeline — Rolling Shutter Correction
- Runtime cache layer — Cache integration for temporal effects
