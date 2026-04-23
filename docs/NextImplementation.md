# Tachyon: Next Implementation

> **📁 This document has been split into focused files in `docs/NextImplementation/` directory:**
> - `00-overview.md` — Progress summary and file structure
> - `tracking.md` — Tracking & Matchmoving
> - `roto_masking.md` — Roto, Masks & Matte Semantics
> - `playback_caching.md` — Caching & Playback
> - `audio_export.md` — Audio Export Pipeline
> - `camera_3d.md` — Camera & 3D Rendering
> - `temporal_effects.md` — Temporal Effects (Time Remap, etc.)
> - `interpolation_graph.md` — Interpolation & Graph Editor
> - `text_animator_pro.md` — Text Animator Pro
> - `editor_support.md` — Editor-facing Support
>
> Read the individual files for detailed implementation status and TODOs.

---

This document is not a feature wishlist.
It is the next layer of implementation needed to turn Tachyon from a deterministic renderer into a usable motion-design and VFX workstation.

The key rule is simple:

- keep core engine semantics deterministic
- keep tracking, masking, timing, and camera behavior first-class in the scene model
- keep AI and editor conveniences as optional accelerators, not hidden dependencies

Many of the items below already exist as architectural intent in the broader docs.
What is still missing is the concrete implementation path and the contract between systems.

---

## Current Codebase State Summary

Before defining gaps, here is what **already exists** in the codebase as of the latest commit:

| System | Status | Location |
|--------|--------|----------|
| Feature Tracker (Harris + LK optical flow) | ✅ Implemented | `include/tachyon/tracker/feature_tracker.h`, `src/tracker/feature_tracker.cpp` |
| Physical Camera Model (focal length, sensor, aperture, focus) | ✅ Implemented | `include/tachyon/core/camera/camera_state.h` |
| Two-Node Camera / Point of Interest | ✅ Implemented | `CameraState::target_position`, `use_target`, `look_at` |
| Camera Rigging / Parenting (Pose + RigGraph) | ✅ Implemented | `include/tachyon/core/scene/rigging/rig_graph.h` |
| Text Animator (per-glyph selectors, coverage, transforms) | ✅ Implemented | `include/tachyon/text/animation/text_animator_pipeline.h` |
| Mask Renderer | ✅ Implemented | `include/tachyon/renderer2d/evaluated_composition/mask_renderer.h` |
| Environment Manager (PBR, IBL, BRDF LUT) | ✅ Implemented | `include/tachyon/renderer3d/lighting/environment_manager.h` |
| Motion Blur (header) | ⚠️ Declared | `include/tachyon/renderer3d/effects/motion_blur.h` |

---

## 1. Tracking and Matchmoving

### What is actually missing

> **Why this matters:** The #1 reason people open After Effects is tracking. 
> "Attach this text to this wall" is 90% of motion design. Without tracking, users must leave Tachyon, track in AE, export, and import — you have already lost.

Tachyon has a **solid feature tracker foundation** (`FeatureTracker` with Harris corner detection, multi-level LK optical flow, pyramid tracking, affine/homography estimation). What is missing is the **pipeline layer** that turns tracked features into production workflows.

The missing pieces are:

- a **track data model** and binding system to drive layer transforms from tracked points
- a **planar tracker** resource type built on top of the existing feature tracker
- a **3D camera solver** for real footage matchmove (separate from the 2D feature tracker)
- **stabilization** and rolling-shutter-aware correction

### Correct implementation path

#### A. 2D point tracker binding system

The `FeatureTracker` already detects and tracks points. What is missing is the **track resource** and the **binding to animatable properties**.

Use case:

- drive a layer transform from tracked points
- attach text or icons to a moving object
- create simple motion-follow workflows without leaving Tachyon

Implementation rules:

- store track samples as time-stamped 2D observations in a dedicated `Track` resource
- expose tracking as a property source, not as a one-off effect
- support at least translation, rotation, and scale solve from a point set
- make the result bindable to any animatable property through the property model

Concrete implementation:

```cpp
// include/tachyon/tracker/track.h
struct TrackSample {
    float time;
    Point2f position;
    float confidence;
};

struct Track {
    std::string name;
    std::vector<TrackSample> samples;
    // Derived: solved transform properties at each sample
    std::optional<std::vector<float>> affine_per_sample;
};

// Binding: connects a Track to an animatable property
class TrackBinding {
public:
    enum class TargetProperty { Position, Rotation, Scale, CornerPin };
    void apply(const Track& track, float time, scene::EvaluatedLayerState& layer);
};
```

Suggested integration:

- `include/tachyon/tracker/track.h` — track data model
- `src/tracker/point_tracker.cpp` — high-level point tracker workflow (detect → track → solve → bind)
- `src/tracker/track_binding.cpp` — property binding logic

#### B. Planar tracker

The planar tracker should be built **on top of `FeatureTracker`**, using its homography output, not around a brittle full-frame warp guess.

Use case:

- stick text to a wall
- replace a screen
- stabilize a signboard or poster
- drive a planar corner-pin or surface insert

Implementation rules:

- use `FeatureTracker::detect_features()` + `track_frame()` + `estimate_homography()` as the engine
- solve a homography per frame with robust outlier rejection (add RANSAC wrapper around the existing DLT)
- optionally refine the plane with a mesh or patch-level update
- expose the final plane as a reusable `PlanarTrack` resource

Important correction:

- do not make non-rigid tracking the first requirement
- start with rigid planar surfaces
- add mesh refinement only after the rigid planar case is stable

Concrete implementation:

```cpp
// include/tachyon/tracker/planar_track.h
struct PlanarTrack {
    std::vector<std::vector<float>> homography_per_frame; // 3x3, from FeatureTracker
    std::vector<float> frame_times;
    
    // Convert to corner-pin for compositor
    std::array<Point2f, 4> corner_pin_at(float time) const;
};

// RANSAC wrapper for robust homography
std::optional<std::vector<float>> estimate_homography_ransac(
    const std::vector<Point2f>& src,
    const std::vector<TrackPoint>& dst,
    int iterations = 100,
    float threshold = 3.0f);
```

Suggested integration:

- extend `tracker/feature_tracker` with RANSAC outlier rejection
- add a `PlanarTrack` resource type
- allow planar tracks to drive corner pin, transform, or matte placement

#### C. 3D camera solver

This should be a **separate matchmoving pipeline**, not a runtime camera trick. It consumes 2D tracks and produces a 3D camera path.

Use case:

- import footage
- estimate camera motion
- reconstruct a scene camera path
- export it into Tachyon's 3D scene graph

Implementation rules:

- solve camera pose per frame from sparse tracked features (bundle adjustment or PnP)
- estimate focal length before exposing the solution to the renderer
- support at least radial distortion parameters in the solver output
- store the result as a camera track that can be edited later

Do not couple the solver to rendering internals.
The solver should produce evaluated camera keyframes or a camera clip that the scene model can consume.

Concrete implementation:

```cpp
// include/tachyon/tracker/camera_solver.h
struct CameraTrackKeyframe {
    float time;
    math::Vector3 position;
    math::Quaternion rotation;
    float focal_length_mm;
    float radial_distortion_k1;
};

struct CameraTrack {
    std::vector<CameraTrackKeyframe> keyframes;
    // Import into scene graph
    void apply_to(tachyon::camera::CameraState& state, float time) const;
};

class CameraSolver {
public:
    // Solve from 2D tracks and optional initial focal length guess
    CameraTrack solve(
        const std::vector<Track>& tracks,
        float sensor_width_mm,
        std::optional<float> initial_focal_length_mm = std::nullopt);
};
```

Suggested integration:

- `include/tachyon/tracker/camera_solver.h`
- `src/tracker/camera_solver.cpp`
- import into `EvaluatedCameraState` and the scene camera track

#### D. Stabilization and rolling shutter correction

Stabilization should be implemented as motion estimation plus transform smoothing.
Rolling shutter correction should be a second pass on top of the stabilization/matchmove data.

Implementation rules:

- estimate global motion first (use `FeatureTracker` + `estimate_affine()` on the full frame)
- smooth the camera path in a deterministic way (use existing `FeatureTracker::smooth_track()` EMA)
- reproject the footage using that stabilized path
- apply rolling-shutter correction only if the source metadata or estimation model supports it

Important correction:

- rolling shutter correction is not the same thing as motion blur
- it should live in the tracking/stabilization pipeline, not in the generic blur pipeline

Concrete implementation:

```cpp
// src/tracker/stabilizer.cpp
struct StabilizationResult {
    std::vector<std::vector<float>> affine_per_frame; // 2x3
    std::vector<float> frame_times;
};

StabilizationResult stabilize(
    const std::vector<GrayImage>& frames,
    float smooth_alpha = 0.7f);

// src/tracker/rolling_shutter.cpp
struct RollingShutterParams {
    float readout_time_ms; // e.g. 30ms for CMOS
    int scan_direction; // 1 = top-to-bottom, -1 = bottom-to-top
};

// Correct per-scanline distortion using per-frame motion
void apply_rolling_shutter_correction(
    const StabilizationResult& motion,
    const RollingShutterParams& params,
    std::vector<GrayImage>& frames);
```

Suggested integration:

- `src/tracker/stabilizer.cpp`
- `src/tracker/rolling_shutter.cpp`

---

## 2. Roto, Masks, and Matte Semantics

### What is actually missing

Tachyon has a `MaskRenderer` but needs a **clean alpha workflow** with variable feather and matte semantics.
The missing parts are:

- variable-feather bezier masks (per-vertex feather, not post-blur)
- AI-assisted roto propagation
- explicit track matte and set matte semantics

### Correct implementation path

#### A. Variable-feather bezier masks

The mask system should stay vector-native and deterministic.

Implementation rules:

- keep a mask as a path plus per-segment or per-vertex feather data
- support feather expansion separately from path geometry
- evaluate the mask stack inside the compositing pipeline, not in a special-case layer drawer
- ensure feathering participates in cache keys and invalidation

Do not model feather as a post blur only.
That produces the wrong edge shape and breaks compositing semantics.

Concrete implementation:

```cpp
// Extend existing mask path data model
struct MaskVertex {
    Point2f position;
    Point2f in_tangent;
    Point2f out_tangent;
    float feather_in{0.0f};  // inner feather radius
    float feather_out{0.0f}; // outer feather radius
};

struct MaskPath {
    std::vector<MaskVertex> vertices;
    bool closed{true};
};

// Mask rasterizer generates inner/outer alpha ramps
// Inner: 1.0 → 0.0 over feather_in
// Outer: 0.0 → 1.0 over feather_out
// Result: inner_ramp * outer_ramp
```

Suggested integration:

- extend the mask path data model with per-vertex feather metadata
- update `MaskRenderer` to generate inner/outer alpha ramps
- preserve deterministic edge evaluation across frames

#### B. AI roto brush

The AI path should be optional and should output a normal matte sequence.

Implementation rules:

- treat AI segmentation as a matte generator, not as a special render mode
- use optical flow or tracker-assisted propagation between frames
- write results into the same matte model used by masks and track mattes
- allow manual cleanup after the AI pass

Important correction:

- SAM2 or any similar model should be an accelerator
- it must not become a hard runtime dependency for the whole app

Concrete implementation:

```cpp
// src/ai/roto_brush.cpp
class RotoBrush {
public:
    struct Config {
        std::string model_path; // SAM2 or similar, optional
        float propagation_threshold{0.5f};
    };
    
    // Generate matte from user scribble + optional AI
    std::vector<float> generate_matte(
        const GrayImage& frame,
        const std::vector<Point2f>& scribble_points,
        const Config& cfg);
    
    // Propagate to next frame using optical flow from FeatureTracker
    std::vector<float> propagate(
        const std::vector<float>& prev_matte,
        const std::vector<TrackPoint>& flow_vectors);
};
```

Suggested integration:

- `src/ai/roto_brush.cpp`
- `src/ai/segmentation_provider.cpp` — abstract provider (can be SAM2 or fallback)
- `src/tracker/optical_flow_seed.cpp` — flow-assisted propagation

#### C. Track matte and set matte

Matte dependencies must be explicit in the render graph.

Implementation rules:

- allow alpha and luma mattes
- support inverted matte modes
- allow a layer to consume a matte source without relying on strict timeline ordering
- resolve matte sources before final compositing of the target layer

This belongs in the compositing pipeline, not in layer order hacks.

Concrete implementation:

See `include/tachyon/core/spec/schema/contracts/shared_contracts.h` for `MatteMode` and `MatteDependency`.

```cpp
// src/composite/matte_resolver.cpp
class MatteResolver {
public:
    // Resolve matte sources in dependency order
    void resolve(
        const std::vector<scene::EvaluatedLayerState>& layers,
        std::vector<float>& output_alpha);
};
```

Suggested integration:

- matte resolver in `src/composite/`
- explicit matte dependency edges in the graph planner

---

## 3. Caching and Playback

### What is actually missing

Tachyon already has deterministic execution intent.
What is missing is a playback feel that remains fast under real editing pressure.

> **Critical insight from AE comparison:** After Effects wins because it is slow but *predictable*. 
> Tachyon must be *fast AND predictable*. If every edit requires 3 seconds of render, no one will use it.
> The "feel" of the editor is more important than raw throughput.

The missing pieces are:

- a disk cache that survives session boundaries
- automatic proxy generation and selection
- RAM preview with audio scrub support
- visible cache diagnostics (what hit, what missed, and why)

### Correct implementation path

#### A. Smart disk cache

The cache should be content-addressed by visible dependencies, not by an approximate tick alone.

Implementation rules:

- key cache entries by scene identity, evaluated dependencies, frame time, quality tier, and AOV/surface type
- store chunked frame results on disk
- invalidate only the subgraph that changed
- make cache hits observable in diagnostics

Important correction:

- `last_modified_tick` is useful as one input
- it is not sufficient as the only cache identity

Concrete implementation:

```cpp
// include/tachyon/runtime/cache_key_builder.h
struct CacheKey {
    std::string scene_hash;      // hash of scene spec
    std::string node_path;       // subgraph path
    float frame_time;
    int quality_tier;          // 0=draft, 1=preview, 2=final
    std::string aov_type;        // "beauty", "depth", "motion_vectors"
    
    std::string to_string() const;
};

class CacheKeyBuilder {
public:
    CacheKey build(
        const scene::EvaluatedCompositionState& state,
        const std::string& node_path,
        float time,
        int quality);
};

// src/media/cache/disk_cache_manager.cpp
class DiskCacheManager {
public:
    void store(const CacheKey& key, const std::vector<uint8_t>& frame_data);
    std::optional<std::vector<uint8_t>> load(const CacheKey& key);
    void invalidate(const std::string& node_path_prefix);
    
    struct Stats {
        size_t hits{0};
        size_t misses{0};
        size_t evicted{0};
    };
    Stats stats() const;
};
```

Suggested integration:

- `src/media/cache/disk_cache_manager.cpp`
- `include/tachyon/runtime/cache_key_builder.h`
- `src/runtime/frame_cache.cpp`

#### B. Proxy workflow

Proxy handling should be part of media ingest and playback policy.

Implementation rules:

- generate proxy assets asynchronously
- preserve a stable mapping from original media to proxy variants
- switch between proxy and original based on playback or render mode
- keep codec choice and resolution policy explicit

Do not hardcode one proxy codec as the architecture.
The system should allow a proxy policy, not a single container assumption.

Concrete implementation:

```cpp
// src/media/asset/proxy_manifest.cpp
struct ProxyVariant {
    std::string original_path;
    std::string proxy_path;
    int width;
    int height;
    std::string codec; // "h264", "prores_proxy", etc.
    float bitrate_mbps;
};

class ProxyManifest {
public:
    void register_proxy(const ProxyVariant& variant);
    std::string resolve_for_playback(const std::string& original, int target_width) const;
    std::string resolve_for_render(const std::string& original) const; // always original
};

// src/media/importer/proxy_worker.cpp
class ProxyWorker {
public:
    // Async generation
    void generate_proxies(
        const std::vector<std::string>& originals,
        const ProxyPolicy& policy);
};
```

Suggested integration:

- `src/media/importer/proxy_worker.cpp`
- `src/media/asset/proxy_manifest.cpp`
- playback policy on the media layer

#### C. RAM preview with audio scrub

RAM preview should be a playback cache, not just a larger frame queue.

Implementation rules:

- keep decoded frame buffers in memory when budget allows
- keep a synchronized audio ring buffer
- allow fast scrubbing without forcing full re-decode
- keep audio resampling deterministic

Concrete implementation:

```cpp
// src/runtime/playback/playback_queue.cpp
class PlaybackQueue {
public:
    void preload(float start_time, float end_time, int quality_tier);
    std::optional<FrameBuffer> get_frame(float time);
    void purge_before(float time); // keep only recent frames
};

// src/runtime/playback/audio_preview_buffer.cpp
class AudioPreviewBuffer {
public:
    void fill(const AudioDecoder& decoder, float start_time, float duration);
    // Scrub: play audio at variable speed without pitch shift
    std::vector<float> resample_for_scrub(float time, float speed);
};
```

Suggested integration:

- `src/runtime/playback/playback_queue.cpp`
- `src/runtime/playback/audio_preview_buffer.cpp`

---

## 4. Camera and 3D

### What is actually missing

Tachyon has **camera transforms, physical camera model, and rigging** already implemented. What is still missing is the **rendering layer** that uses these camera properties for cinematic output.

The current gap compared to After Effects and professional motion-design tools:

| Feature | Why it matters | Status in Tachyon |
|---------|---------------|-------------------|
| Point of Interest (POI) | Essential for motion design (e.g., orbit around a logo). | ✅ **IMPLEMENTED** in `CameraState::target_position` + `use_target` |
| Physical Camera Model | Focal Length (50mm, 35mm), sensor size, aperture. | ✅ **IMPLEMENTED** in `CameraState` |
| Depth of Field (DoF) | Cinematic bokeh. Requires Z-buffer + lens blur. | ❌ Missing (has aperture param but no DoF pass) |
| Camera Parenting | Attach camera to Null for rigs. | ✅ **IMPLEMENTED** in `RigGraph` + `Pose` |
| Camera Shake / Noise | Simulate handheld footage. | ❌ Missing |
| Motion Blur (Shutter Angle) | Cinematic motion blur with sub-frame sampling. | ⚠️ Header exists, implementation needed |
| Multi-View Viewport | Top, Side, Front views for 3D editing. | ❌ Missing |
| Unified 2D/3D Depth Sorting | 2D layers in real Z-space occluding 3D objects. | ⚠️ Planned |
| Lights and Shadows | Directional, point, spot lights + shadow mapping. | ⚠️ EnvironmentManager exists but no light types/shadows |

The missing pieces are:

- depth of field (using existing aperture/focus_distance)
- camera shake and procedural motion
- multi-camera timeline cuts
- 2D and 3D depth-aware integration
- lights and shadows as part of the real 3D path
- motion blur with shutter-angle semantics
- multi-view viewport for 3D editing

### Correct implementation path (5 phases)

> **Note:** Many camera features (POI, physical camera model, parenting) are already implemented in the data model and evaluator. The work ahead is wiring, testing, and rendering-layer integration.

#### Phase 1 — Consolidate Camera/Evaluator (test & consistency)

**Status:** Data model exists. Needs verification and golden tests.

What to verify:
- `point_of_interest` is read from JSON spec and passed to `EvaluatedCameraState`
- `focal_length_mm` correctly overrides `fov_y_rad` in `CameraState::get_projection_matrix()`
- `is_two_node: true` with a target produces correct orbit behavior without manual Euler math
- Parenting through `RigGraph` + `Pose` correctly propagates world transforms to camera

Test target: `tests/scenes/camera_two_node.json` — a camera orbiting a centered cube.

Files: `src/core/scene/evaluator.cpp`, `include/tachyon/core/camera/camera_state.h`

---

#### Phase 2 — Fix Motion Blur with real subframe evaluation

**Status:** `MotionBlurRenderer` has structure but copies the same camera state for every subframe.

Problem: `generate_subframe_states()` does not interpolate camera or object matrices between previous and current frame.

What to implement:
1. Add `previous_camera_matrix` to `EvaluatedCameraState` (currently only `previous_position`/`previous_point_of_interest` exist)
2. In `generate_subframe_states()`, interpolate camera matrix between previous and current for each subframe time
3. For each 3D object, interpolate `world_matrix` using `previous_world_matrix` (already in `EvaluatedLayerState`)
4. Feed the interpolated scene to the ray tracer per subframe, not the base scene repeated

Files: `src/renderer3d/animation/motion_blur.cpp`, `src/core/scene/evaluator.cpp`

---

#### Phase 3 — Depth of Field (DoF) as post-pass

**Status:** `depth_z` AOV already produced by `ray_tracer.cpp`. DoF blur pass is missing.

What to implement:
1. Create `src/renderer3d/effects/depth_of_field.cpp`
2. Circle of Confusion (CoC) per pixel from `depth_z`, `focus_distance`, `aperture_fstop`, `focal_length_mm`
3. Quality tiers:
   - Preview: Gaussian blur weighted by CoC
   - Final: Bokeh disc convolution (hexagonal if `aperture_blades > 0`)
4. Integrate after ray tracer, before 2D compositing

Files: `src/renderer3d/effects/depth_of_field.cpp`, `include/tachyon/renderer3d/effects/depth_of_field.h`

---

#### Phase 4 — Camera Shake (deterministic)

**Status:** Not implemented.

What to implement:
1. Add `CameraShake` struct to camera properties:
   - `seed` (uint64), `amplitude_position` (Vector3), `amplitude_rotation` (Vector3), `frequency` (float)
2. Use seeded deterministic noise (e.g. simplex or precomputed table) to generate offset at time t
3. Apply in evaluator before view matrix computation: `position += shake.evaluate(t)`
4. Must be reproducible: same seed + same time = same shake

Files: `include/tachyon/core/camera/camera_shake.h`, `src/core/scene/evaluator.cpp`

---

#### Phase 5 — Multi-Camera Timeline Cuts

**Status:** Not implemented.

What to implement:
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

Files: `src/core/spec/layer_parse_json.cpp`, `src/core/scene/evaluator.cpp`

---

#### Deferred (after Phase 5)

- **Lights & Shadows:** Already iterate in `ray_tracer_shading.cpp` with Disney BSDF. Verify `EvaluatedLightState` population in evaluator is complete.
- **2D/3D Depth Sorting:** Requires 2D layers with `is_3d=true` to become textured quads in the 3D ray tracer. High architectural risk — do only after DoF is stable.
- **Multi-View Viewport:** Editor-only, depends on core camera contracts above.
- **Coordinate System Documentation:** Cleanup task, not a product feature.

#### D. 2D and 3D depth-aware integration

If a 2D layer lives in a 3D scene, it needs a real depth contract.

Implementation rules:

- allow 2D layers to occupy 3D space when requested
- use a unified depth-aware render path
- preserve occlusion semantics between 2D and 3D elements

This is the point where Tachyon can outperform simple 2.5D stacks.

Concrete implementation:

```cpp
// 2D layer in 3D space: assign a Z position and transform
struct Layer3DPlacement {
    math::Vector3 position;
    math::Quaternion rotation;
    math::Vector3 scale{1,1,1};
    bool enable_depth_test{true};
};

// Unified render: sort all elements (2D + 3D) by depth, then render back-to-front
// or use Z-buffer for opaque elements
```

#### E. Lights and shadows

Tachyon's 3D path needs actual lighting support, not only camera math.
`EnvironmentManager` handles IBL but explicit light types are missing.

Implementation rules:

- add explicit light types such as directional, point, and spot
- make shadow casting part of the 3D render pipeline
- keep ambient occlusion and shadow cost tiered
- ensure lights participate in the same scene evaluation model as cameras

Concrete implementation:

```cpp
// include/tachyon/scene/light_state.h
enum class LightType { Directional, Point, Spot };

struct LightState {
    LightType type;
    math::Vector3 position;
    math::Vector3 direction;
    math::Vector3 color{1.0f, 1.0f, 1.0f};
    float intensity{1.0f};
    float range{10.0f}; // for point/spot
    float spot_angle_deg{45.0f}; // for spot
    bool cast_shadows{false};
};

// Shadow map generation
// src/renderer3d/shadows/shadow_map.cpp
class ShadowMapGenerator {
public:
    std::vector<float> render_shadow_map(
        const LightState& light,
        const std::vector<media::MeshAsset>& meshes);
};
```

Suggested integration:

- `include/tachyon/scene/light_state.h`
- `src/renderer3d/lights/light_evaluator.cpp`
- `src/renderer3d/shadows/shadow_map.cpp`

#### F. Motion blur with shutter angle

Motion blur should be implemented as sub-frame sampling.

Implementation rules:

- sample the scene across the shutter interval
- accumulate the resulting frames deterministically
- respect camera, layer, and object motion equally
- keep sample count and shutter policy visible in the render contract

Concrete implementation:

```cpp
// include/tachyon/renderer3d/effects/motion_blur.h (already exists — implement)
struct MotionBlurParams {
    float shutter_angle_deg{180.0f}; // 180° = half frame duration
    int sample_count{8}; // 8 for preview, 16+ for final
    uint32_t seed{0}; // for deterministic jitter
};

// Generate sub-frame times: [-shutter/2, +shutter/2] relative to frame center
std::vector<float> generate_subframe_times(float frame_time, float frame_duration, const MotionBlurParams& params);
```

---

## 5. Temporal Effects

### What is actually missing

> **Why this matters:** Modern motion design is not just about frames — it's about *time feel*. 
> CapCut wins users with fluid speed ramps. AE's Time Remapping with optical flow is the standard for professional speed ramps. 
> Without time remapping + frame blending, 24fps footage looks choppy and amateur.

Tachyon needs stronger time-domain behavior for modern motion design.

The missing pieces are:

- optical-flow time remapping
- frame blending / pixel motion
- motion blur tied to the same temporal sampling model
- a clear distinction between retiming and blur

### Correct implementation path

#### A. Optical flow time remap

Use optical flow as a retiming assist for speed ramps and in-between frame synthesis.

Implementation rules:

- evaluate source motion between adjacent frames
- warp the image through the motion field
- blend or synthesize intermediate frames deterministically
- fall back to simple hold or blend when the flow confidence is poor

Do not make optical flow the only retiming path.
The engine should still support clean time remap without AI or flow when the shot does not need it.

Concrete implementation:

See `include/tachyon/core/spec/schema/contracts/shared_contracts.h` for `TimeRemapCurve` and `TimeRemapMode`.

```cpp
// src/timeline/time_remap.cpp

// Optical flow assisted synthesis
std::vector<float> synthesize_frame_optical_flow(
    const std::vector<float>& frame_a,
    const std::vector<float>& frame_b,
    const std::vector<std::pair<float, float>>& flow_ab, // (dx, dy) per pixel
    float t); // interpolation factor [0, 1]
```

Suggested integration:

- `src/timeline/time_remap.cpp`
- `src/timeline/frame_blend.cpp`
- optional flow-assisted synth pass

#### B. Frame blending and pixel motion

Frame blending should be a cheaper retiming fallback than full synthesis.

Implementation rules:

- support hold, blend, and motion-assisted modes
- keep the selected method explicit in the timeline model
- include it in cache identity

#### C. Motion blur

Motion blur should be implemented as sub-frame sampling, which is already the right direction in the existing docs.
See Section 4.F for concrete implementation.

#### D. Rolling shutter

Rolling shutter is a separate temporal distortion model.

Implementation rules:

- model scanline timing explicitly
- apply correction or simulation as a dedicated pass
- keep it independent from motion blur exposure logic

See Section 1.D for concrete implementation.

---

## 6. Interpolation and Graph Editing

### What is actually missing

> **Why this matters:** CapCut and AE excel at fluid animation (Ease-In, Ease-Out, S-curves). 
> If Tachyon handles only linear interpolation (A → B at constant speed), the result will always look amateur. 
> The Graph Editor is not a separate animation system — it is a *view* on top of the same animation core.

Tachyon needs one interpolation core that can drive properties, camera parameters, mask controls, text selectors, and time remap curves.

The missing pieces are:

- a universal cubic-bezier interpolator
- value graphs and speed graphs
- spatial interpolation for position curves

### Correct implementation path

#### A. Universal bezier interpolation

The interpolation engine should live under the property model, not as a timeline-only helper.

Implementation rules:

- support linear, hold, and bezier-based easing
- evaluate all keyframe curves through the same property API
- keep tangent and easing data deterministic
- make cache keys reflect the curve shape when visible output depends on it

Concrete implementation:

```cpp
// include/tachyon/core/properties/bezier_interpolator.h
enum class InterpolationType { Hold, Linear, Bezier };

struct Keyframe {
    float time;
    float value;
    float in_tangent{0.0f};  // slope incoming
    float out_tangent{0.0f}; // slope outgoing
    InterpolationType type{InterpolationType::Linear};
};

class BezierInterpolator {
public:
    // Evaluate at time t using cubic bezier between keyframes
    static float evaluate(const std::vector<Keyframe>& keyframes, float t);
    
    // Convert between value graph and speed graph
    static std::vector<Keyframe> value_to_speed_graph(const std::vector<Keyframe>& value_graph);
    static std::vector<Keyframe> speed_to_value_graph(const std::vector<Keyframe>& speed_graph);
};
```

#### B. Graph editor backend

The graph editor should consume the same evaluation primitives that the engine uses.

Implementation rules:

- expose value graph and speed graph data
- support tangent editing
- convert between speed and value space explicitly
- allow spatial bezier paths for transforms when the property type supports it

Important correction:

- the graph editor is not a separate animation system
- it is a view on top of the same animation core

---

## 7. Text Animator Pro

### What is actually missing

Text layout alone is not enough.
Tachyon has a **solid per-glyph animation pipeline** (`TextAnimatorPipeline` with selectors, coverage, position/scale/rotation/opacity/fill/stroke animation). What is still missing is:

- text on path
- range selectors with more advanced selection modes (word, line, random)

### Correct implementation path

#### A. Glyph-level selectors

✅ **ALREADY IMPLEMENTED** in `TextAnimatorPipeline`:
- Per-glyph coverage computation
- Position, scale, rotation, opacity animation
- Fill color, stroke color, stroke width animation
- Tracking/spacing animation

#### B. Per-character animation

✅ **ALREADY IMPLEMENTED** — see above.

#### C. Text on path

Text on path should be implemented as a layout mode, not as a post-warp.

Implementation rules:

- parameterize the path
- place glyphs along the path curve
- support orientation controls like perpendicular alignment
- keep shaping and animation separable

Concrete implementation:

```cpp
// include/tachyon/text/animation/text_on_path.h
struct PathSpec {
    std::vector<Point2f> points; // Bezier control points
    bool closed{false};
};

class TextOnPathLayout {
public:
    // Place glyphs along path, return modified positions
    std::vector<GlyphPlacement> layout_along_path(
        const std::vector<GlyphPlacement>& base_glyphs,
        const PathSpec& path,
        float start_offset{0.0f});
};
```

Suggested integration:

- `src/text/animation/text_on_path.cpp`
- integrate into `TextAnimatorPipeline` as a pre-animation layout step

---

## 8. Editor-facing support that should not be forgotten

These are not always core render features, but they are necessary for the engine to feel usable.

- multi-view 3D viewport: top, side, front, and camera views for rigging
- visible cache diagnostics: what hit, what missed, and why
- scene graph inspection: parents, camera targets, matte links, and track bindings
- proxy and playback mode indicators

These should sit in the editor layer, but they depend on the core contracts above.

Concrete implementation for viewport:

```cpp
enum class ViewportMode { Perspective, Top, Side, Front };

struct ViewportCamera {
    ViewportMode mode;
    math::Vector3 position;
    math::Vector3 target;
    float ortho_size{10.0f}; // for orthographic views
};

// Generate view/projection matrices for each viewport mode
math::Matrix4x4 viewport_view_matrix(const ViewportCamera& cam);
math::Matrix4x4 viewport_projection_matrix(const ViewportCamera& cam, float aspect);
```

---

## Recommended order

1. **lock the tracking, matte, camera, and time contracts** — define headers and data models
2. **implement point tracker binding, planar tracker, and camera solver** — build on existing `FeatureTracker`
3. **finish mask feather and matte evaluation** in the compositing graph — extend existing `MaskRenderer`
4. **make cache and proxy behavior visible and deterministic** — disk cache + RAM preview
5. **add DoF, camera shake, and camera cut support** — use existing camera params
6. **complete time remap, frame blending, and motion blur integration** — temporal pipeline
7. **ship the graph editor backend and text on path** — animation curves + text layout
8. **layer AI roto and rolling shutter correction** on top of stable primitives

## Guiding rule

If a feature does not improve deterministic output, editability, playback feel, or shot-level realism, it should not be treated as a core dependency.
