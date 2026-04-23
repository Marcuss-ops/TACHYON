# Interpolation & Graph Editor

> **Why this matters:** CapCut and AE excel at fluid animation (Ease-In, Ease-Out, S-curves).
> If Tachyon handles only linear interpolation (A → B at constant speed), result will always look amateur.
> The Graph Editor is not a separate animation system — it is a *view* on top of the same animation core.

## Current State

Tachyon needs one interpolation core that can drive properties, camera parameters, mask controls, text selectors, and time remap curves.

**What exists:**
- ✅ `include/tachyon/core/properties/bezier_interpolator.h` (value + speed graph helpers)
- ✅ `include/tachyon/core/properties/keyframe_track.h`
- ✅ `src/core/properties/bezier_interpolator.cpp`

**What is missing:**
- Graph editor backend (value/speed graph conversion)
- Spatial interpolation for position curves
- Expose graph data for editor UI
- Tangent editing support
- Scene contract for sharing curve IDs and property paths with editor/runtime

---

## 1. Universal Bezier Interpolation

**Status:** ✅ Mostly implemented, needs enhancement

### Current Implementation
```cpp
// From include/tachyon/core/properties/bezier_interpolator.h
enum class InterpolationType { Hold, Linear, Bezier };

struct Keyframe {
    float time;
    float value;
    float in_tangent{0.0f};
    float out_tangent{0.0f};
    InterpolationType type{InterpolationType::Linear};
};

class BezierInterpolator {
public:
    static float evaluate(const std::vector<Keyframe>& keyframes, float t);
};
```

### What to Add
- [x] **Value graph ↔ Speed graph conversion**
- [ ] **Spatial bezier paths** for transform properties (position X/Y/Z)
- [ ] **Ease-in/ease-out presets** (pre-built tangent configurations)
- [ ] **Weighted tangents** (independent in/out tangent lengths)

### Files to Modify
```
include/tachyon/core/properties/bezier_interpolator.h  # ADD: graph conversion methods
src/core/properties/bezier_interpolator.cpp            # ADD: implementation
docs/NextImplementation/scene_contracts.md             # ADD: shared property path references
```

### Planned Enhancements
```cpp
class BezierInterpolator {
public:
    // Existing: evaluate at time t
    static float evaluate(const std::vector<Keyframe>& keyframes, float t);
    
    // NEW: Convert between value graph and speed graph
    static std::vector<Keyframe> value_to_speed_graph(const std::vector<Keyframe>& value_graph);
    static std::vector<Keyframe> speed_to_value_graph(const std::vector<Keyframe>& speed_graph);
    
    // NEW: Apply ease presets
    static void apply_ease_in(std::vector<Keyframe>& keyframes, int index);
    static void apply_ease_out(std::vector<Keyframe>& keyframes, int index);
    static void apply_ease_in_out(std::vector<Keyframe>& keyframes, int index);
};
```

---

## 2. Graph Editor Backend

**Status:** ⚠️ Not implemented in editor layer

### Implementation Rules
- **Graph editor is not a separate animation system**
- It is a **view** on top of the same animation core
- Expose value graph and speed graph data
- Support tangent editing
- Convert between speed and value space explicitly
- Allow spatial bezier paths for transforms when property type supports it
- Use the same property identifiers as the scene contract

### Files to Create
```
src/core/properties/graph_editor_backend.cpp   # NEW
include/tachyon/core/properties/graph_editor_backend.h  # NEW
```

### Data Model (Planned)
```cpp
struct GraphPoint {
    float time;
    float value;
    math::Vector2 in_tangent;  // relative to point
    math::Vector2 out_tangent; // relative to point
    bool weighted{false};
};

enum class GraphType { Value, Speed, Spatial };

class GraphEditorBackend {
public:
    // Get graph data for a property track
    std::vector<GraphPoint> get_value_graph(const std::string& property_path) const;
    std::vector<GraphPoint> get_speed_graph(const std::string& property_path) const;
    
    // Tangent editing
    void set_tangent(const std::string& property_path, int keyframe_index, 
                     const math::Vector2& in, const math::Vector2& out);
    
    // Spatial paths (for position, anchor point, etc.)
    std::vector<math::Vector3> get_spatial_path(const std::string& property_path) const;
    
    // Convert selection to graph (for text animators, track mattes, etc.)
    std::vector<GraphPoint> evaluate_for_display(float start_time, float end_time, int sample_count);
};
```

---

## 3. Spatial Interpolation for Position

**Status:** ❌ Not implemented

### Use Cases
- Smooth spatial paths for position keyframes
- Separate X, Y, Z curves vs. unified spatial bezier
- Match AE's spatial interpolation behavior

### Implementation Rules
- Support both **per-channel** (X, Y, Z separate) and **spatial** (3D bezier path) modes
- Allow switching between modes per property
- Store spatial bezier control points separately from value keyframes

### Integration with Text Animator
- Text animators (see `text_animator_pro.md`) can use spatial paths for per-glyph animation

### Files to Modify/Create
```
include/tachyon/core/properties/spatial_keyframe.h  # NEW
src/core/properties/spatial_keyframe.cpp             # NEW
```

### Data Model (Planned)
```cpp
struct SpatialKeyframe {
    float time;
    math::Vector3 position;
    math::Vector3 in_tangent;  // spatial tangent
    math::Vector3 out_tangent; // spatial tangent
    bool linear{false};        // if true, skip bezier, use linear spatial interpolation
};

class SpatialInterpolator {
public:
    static math::Vector3 evaluate(
        const std::vector<SpatialKeyframe>& keyframes, 
        float t);
    
    // Get 3D path for visualization
    std::vector<math::Vector3> get_path(
        const std::vector<SpatialKeyframe>& keyframes,
        int segments_per_segment = 20);
};
```

---

## Integration Points

### With Property Model
- All animatable properties use `KeyframeTrack<T>` (existing)
- Graph editor reads from same tracks it animates

### With Text Animator (see `text_animator_pro.md`)
- Text selector ranges can be animated via graph editor
- Per-glyph transforms (position, scale, rotation) can use spatial paths

### With Tracking (tracking pipeline)
- Track bindings (position, rotation, scale) can be edited in graph editor
- Speed graph useful for analyzing track smoothness

### With Camera (see `camera_3d.md`)
- Camera shake parameters (amplitude, frequency) can be keyframed
- Camera cuts can be visualized in time graph

---

## Implementation Priority
1. ✅ Enhance `BezierInterpolator` with graph conversion methods
2. ⬜ Implement `GraphEditorBackend` (value/speed graph exposure)
3. ⬜ Implement spatial interpolation for position properties
4. ⬜ Add ease-in/out presets and tangent presets
5. ⬜ Integrate graph data with editor UI (depends on editor layer)
6. ⬜ Define curve serialization in the scene contract

---

## Dependencies
- `BezierInterpolator` (exists, needs enhancement)
- `KeyframeTrack<T>` (exists)
- Property model (`src/core/properties/`)
- Expression engine (for graph-driven expressions)

---

## References
- `text_animator_pro.md` — Text animation uses interpolation
- Tracking pipeline — Track bindings can be graphed
- `camera_3d.md` — Camera properties need interpolation
- `temporal_effects.md` — Time remap curves use bezier interpolation
- `scene_contracts.md` — Shared curve IDs and property paths
