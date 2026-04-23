# Camera & 3D Rendering

> **Goal:** Wire up existing camera model, physical camera params, and rigging into cinematic 3D output.

## Current State

Tachyon has **camera transforms, physical camera model, and rigging** already implemented.

**What exists:**
- ✅ `include/tachyon/core/camera/camera_state.h` (physical camera model)
- ✅ `CameraState::target_position`, `use_target` (POI)
- ✅ `include/tachyon/core/scene/rigging/rig_graph.h` (camera parenting)
- ✅ `renderer3d/lighting/environment_manager.h` (IBL, PBR)
- ✅ `renderer3d/core/ray_tracer.cpp` (produces `depth_z` AOV)
- ✅ `src/renderer3d/effects/depth_of_field.cpp` (DoF post-pass)
- ✅ `src/core/camera/camera_shake.cpp` (deterministic camera shake)
- ✅ Camera cut timeline integration (unified camera cut contract)
- ⚠️ `include/tachyon/renderer3d/effects/motion_blur.h` (contract only; implementation still partial)
- ⚠️ `include/tachyon/core/camera/camera_shake.h` (contract exists; evaluator hookup still needs a clean boundary)

**What is missing:**
- Depth of Field tuning and editor control surface
- Camera shake evaluator integration and shot-level controls
- Multi-camera timeline cuts in the scene spec
- Lights & shadows (explicit light types, shadow mapping)
- 2D/3D depth-aware integration
- Motion blur with shutter-angle semantics (complete sub-frame sampling and interpolation)
- Multi-view viewport for 3D editing
- Shared scene contract for camera tracks, cuts, and shake parameters (`scene_contracts.md`)

---

## Phase 1 — Consolidate Camera/Evaluator (Test & Consistency)

**Status:** Data model exists. Needs verification and golden tests.

### What to Verify
- [ ] `point_of_interest` read from JSON spec → `EvaluatedCameraState`
- [ ] `focal_length_mm` correctly overrides `fov_y_rad` in `CameraState::get_projection_matrix()`
- [ ] `is_two_node: true` with target produces correct orbit behavior
- [ ] Parenting through `RigGraph` + `Pose` propagates world transforms correctly

### Test Target
- `tests/scenes/camera_two_node.json` — camera orbiting a centered cube

### Files to Check/Modify
```
src/core/scene/evaluator.cpp
include/tachyon/core/camera/camera_state.h
docs/NextImplementation/scene_contracts.md
tests/integration/golden/camera_two_node_test.cpp
```

---

## Phase 2 — Fix Motion Blur with Real Subframe Evaluation

**Status:** ⚠️ `MotionBlurRenderer` has structure but copies same camera state for every subframe.

### Problem
`generate_subframe_states()` does not interpolate camera or object matrices between previous and current frame.

### What to Implement
1. Add `previous_camera_matrix` to `EvaluatedCameraState`
2. In `generate_subframe_states()`, interpolate camera matrix between previous and current for each subframe time
3. For each 3D object, interpolate `world_matrix` using `previous_world_matrix`
4. Feed interpolated scene to ray tracer per subframe

### Files to Modify
```
src/renderer3d/animation/motion_blur.cpp
src/core/scene/evaluator.cpp
include/tachyon/renderer3d/effects/motion_blur.h  # Complete implementation
```

### Data Model (Existing, Needs Completion)
```cpp
struct MotionBlurParams {
    float shutter_angle_deg{180.0f}; // 180° = half frame duration
    int sample_count{8}; // 8 for preview, 16+ for final
    uint32_t seed{0}; // for deterministic jitter
};

// Generate subframe times: [-shutter/2, +shutter/2] relative to frame center
std::vector<float> generate_subframe_times(float frame_time, float frame_duration, const MotionBlurParams& params);
```

---

## Phase 3 — Depth of Field (DoF) as Post-Pass

**Status:** ✅ Implemented as a post-pass; still needs tuning and editor controls

### What to Implement
1. Create `src/renderer3d/effects/depth_of_field.cpp`
2. Circle of Confusion (CoC) per pixel from `depth_z`, `focus_distance`, `aperture_fstop`, `focal_length_mm`
3. Quality tiers:
   - **Preview:** Gaussian blur weighted by CoC
   - **Final:** Bokeh disc convolution (hexagonal if `aperture_blades > 0`)
4. Integrate after ray tracer, before 2D compositing

### Files to Create/Modify
```
src/renderer3d/effects/depth_of_field.cpp    # NEW
include/tachyon/renderer3d/effects/depth_of_field.h  # NEW
src/renderer3d/core/ray_tracer.cpp           # MODIFY: Ensure depth_z output
```

### Data Model (Planned)
```cpp
struct DepthOfFieldParams {
    float focus_distance{5.0f};    // meters
    float aperture_fstop{2.8f};    // f-stop value
    int aperture_blades{0};        // 0 = circular, >0 = polygonal bokeh
    int quality_tier{1};           // 0=draft, 1=preview, 2=final
};

class DepthOfFieldPass {
public:
    FrameBuffer apply(
        const FrameBuffer& beauty,
        const FrameBuffer& depth_z,
        const DepthOfFieldParams& params);
};
```

---

## Phase 4 — Camera Shake (Deterministic)

**Status:** ⚠️ Implemented in the renderer path; evaluator wiring and UI controls still need work

### What to Implement
1. Add `CameraShake` struct to camera properties:
   - `seed` (uint64), `amplitude_position` (Vector3), `amplitude_rotation` (Vector3), `frequency` (float)
2. Use seeded deterministic noise (simplex or precomputed table) to generate offset at time t
3. Apply in evaluator before view matrix computation: `position += shake.evaluate(t)`
4. Must be reproducible: same seed + same time = same shake

### Files to Modify/Create
```
include/tachyon/core/camera/camera_shake.h    # COMPLETE: Add evaluate() method
src/core/scene/evaluator.cpp                   # ADD: Apply shake to camera transform
```

### Data Model (Planned Extension)
```cpp
struct CameraShake {
    uint64_t seed{0};
    math::Vector3 amplitude_position{0,0,0};
    math::Vector3 amplitude_rotation{0,0,0}; // radians
    float frequency{10.0f}; // Hz
    
    math::Vector3 evaluate(float time) const; // Returns position offset
    math::Quaternion evaluate_rotation(float time) const;
};
```

---

## Phase 5 — Multi-Camera Timeline Cuts

**Status:** ⚠️ Resolver exists; scene spec wiring still needs to land.

### What to Implement
1. Add `camera_cuts` array to `CompositionSpec`:
   ```json
   "camera_cuts": [
     { "start_time": 0, "end_time": 5, "camera_layer_id": "cam1" },
     { "start_time": 5, "end_time": 10, "camera_layer_id": "cam2" }
   ]
   ```
2. In `evaluate_camera_state()`, resolve active camera by matching `composition_time_seconds` against cuts
3. Fallback to first active camera if no cut matches (backward compatibility)
4. Phase 1: hard cuts only. Phase 2 (optional): blend between cameras

### Files to Modify
```
src/core/spec/layer_parse_json.cpp    # Parse camera_cuts from JSON
src/core/scene/evaluator.cpp           # Resolve active camera from cuts
include/tachyon/core/scene/composition_spec.h  # Add camera_cuts field
```

---

## Deferred (After Phase 5)

### Lights & Shadows
- ✅ Iterate in `ray_tracer_shading.cpp` with Disney BSDF
- [ ] Verify `EvaluatedLightState` population in evaluator is complete
- [ ] Add explicit light types: directional, point, spot
- [ ] Implement shadow mapping or ray-traced shadows

### 2D/3D Depth Sorting
- [ ] 2D layers with `is_3d=true` become textured quads in 3D ray tracer
- **High architectural risk** — do only after DoF is stable

### Multi-View Viewport
- Editor-only feature, depends on core camera contracts above
- `enum class ViewportMode { Perspective, Top, Side, Front };`
- Viewport state should read from the shared scene contract, not from ad-hoc editor-only fields

---

## Implementation Priority
1. ⬜ Phase 1: Verify camera POI, focal length, parenting (tests)
2. ⬜ Phase 2: Complete Motion Blur sub-frame interpolation
3. ⬜ Phase 3: Implement Depth of Field post-pass
4. ⬜ Phase 4: Implement Camera Shake (deterministic noise)
5. ⬜ Phase 5: Multi-Camera Timeline Cuts
6. ⬜ Lights & Shadows (explicit types + shadow mapping)
7. ⬜ 2D/3D Depth Sorting
8. ⬜ Multi-View Viewport

---

## Dependencies
- `CameraState` (exists)
- `FeatureTracker` (for camera solve, see the tracking pipeline)
- `RayTracer` + `depth_z` AOV (exists)
- `EvaluatedCameraState` (exists, needs `previous_camera_matrix`)
- `RigGraph` + `Pose` (exists for parenting)
- `scene_contracts.md` for camera cut, shake, and track references
