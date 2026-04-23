# Editor-Facing Support

> **Note:** These are not always core render features, but they are necessary for the engine to feel usable.
> These should sit in the editor layer, but they depend on core contracts from other documents.

## Current State

Editor features depend on stable core contracts from:
- Tracking pipeline
- Masks & Matte (`roto_masking.md`)
- Runtime cache layer
- Camera & 3D (`camera_3d.md`)
- Interpolation (`interpolation_graph.md`)
- Shared scene contracts (`scene_contracts.md`)

**What is missing (all editor-layer):**
- Multi-view 3D viewport (top, side, front, camera views)
- Visible cache diagnostics (hits, misses, reasons)
- Scene graph inspection (parents, camera targets, matte links, track bindings)
- Proxy and playback mode indicators

---

## 1. Multi-View 3D Viewport

**Status:** ❌ Not implemented

### Use Cases
- Rigging cameras and objects in orthographic views
- Precise 3D placement (top view for floor alignment)
- Match AE's viewport layout (perspective + 3 ortho views)

### Implementation Rules
- Support Perspective, Top, Side, Front viewport modes
- Generate appropriate view/projection matrices per mode
- Orthographic views for top/side/front
- Perspective for camera views

### Files to Create (Editor Layer)
```
editor/viewport/viewport_camera.h      # NEW: Viewport camera modes
editor/viewport/viewport_renderer.h    # NEW: Render scene in viewport mode
editor/viewport/viewport_manager.h     # NEW: Manage multiple viewports
```

### Data Model (Planned)
```cpp
enum class ViewportMode { Perspective, Top, Side, Front };

struct ViewportCamera {
    ViewportMode mode;
    math::Vector3 position;
    math::Vector3 target;
    float ortho_size{10.0f}; // for orthographic views
    
    // Generate matrices
    math::Matrix4x4 view_matrix() const;
    math::Matrix4x4 projection_matrix(float aspect) const;
};
```

### Integration with Core
- Uses `CameraState` for Perspective mode (see `camera_3d.md`)
- Requires camera contracts to be stable (Phase 1-5 in `camera_3d.md`)

---

## 2. Visible Cache Diagnostics

**Status:** ❌ Not implemented (depends on the runtime cache layer)

### Requirements
- Expose cache hit/miss statistics to editor UI
- Show reasons for cache misses (scene change, quality change, invalidation)
- Real-time stats during render session
- Include cache identity details in logs
- Reuse the same diagnostics source used by the runtime cache layer

### Files to Create (Editor Layer)
```
editor/diagnostics/cache_diagnostics.h    # NEW
editor/diagnostics/cache_diagnostics.cpp  # NEW
```

### Data Model (Planned)
```cpp
struct CacheDiagnostics {
    size_t hits{0};
    size_t misses{0};
    size_t evicted{0};
    
    struct MissReason {
        std::string cache_key;
        std::string reason; // "scene_change", "quality_change", "invalidation", etc.
        float time;
    };
    std::vector<MissReason> recent_misses;
    
    void clear();
    std::string to_string() const; // Human-readable report
};
```

### Integration with Core
- Depends on cache stats exposed by the runtime cache layer
- Hook into render session for real-time stats

---

## 3. Scene Graph Inspection

**Status:** ❌ Not implemented

### Requirements
- Visualize scene hierarchy (parents, children)
- Show camera targets (POI links)
- Display matte links (track matte sources/targets)
- Show track bindings (which layers driven by which tracks)
- Editable in-place (click to select, modify properties)
- Include audio tracks and proxy media nodes once the audio pipeline and runtime cache layer land

### Files to Create (Editor Layer)
```
editor/scene_graph/scene_graph_model.h     # NEW: Scene graph data model for UI
editor/scene_graph/scene_graph_view.h       # NEW: UI component
```

### Data Model (Planned)
```cpp
struct SceneGraphNode {
    std::string id;
    std::string display_name;
    std::vector<std::string> children;
    std::string parent; // empty if root
    
    // Camera-specific
    std::optional<std::string> target_id; // POI target
    
    // Matte-specific
    std::optional<MatteDependency> matte_source;
    
    // Track-specific
    std::vector<std::string> bound_tracks; // track IDs driving this layer
};

class SceneGraphModel {
public:
    void build_from(const scene::EvaluatedCompositionState& state);
    const std::vector<SceneGraphNode>& nodes() const;
    
    // Queries
    std::optional<SceneGraphNode> find_node(const std::string& id) const;
    std::vector<SceneGraphNode> children_of(const std::string& id) const;
};
```

### Integration with Core
- Reads from `EvaluatedCompositionState` (existing)
- Depends on track bindings (tracking pipeline)
- Depends on matte semantics (see `roto_masking.md`)

---

## 4. Proxy & Playback Mode Indicators

**Status:** ❌ Not implemented (depends on the runtime cache layer)

### Requirements
- Show current playback mode (draft/preview/final)
- Indicate proxy vs. original media
- Display cache status (cached frames highlighted in timeline)
- Show RAM preview status (active/inactive, percentage filled)

### Files to Create (Editor Layer)
```
editor/playback/playback_indicators.h    # NEW
editor/playback/playback_indicators.cpp  # NEW
```

### Integration with Core
- Reads proxy manifest (runtime cache layer)
- Reads cache status from the runtime cache layer
- Shows RAM preview queue status (runtime cache layer)

---

## Implementation Priority

**These depend on core contracts being stable first!**

1. ⬜ Complete core contracts:
   - [ ] Tracking & bindings (tracking pipeline)
   - [ ] Matte semantics (`roto_masking.md`)
   - [ ] Camera contracts (`camera_3d.md` Phase 1-5)
   - [ ] Cache & proxy (runtime cache layer)
   - [ ] Scene schema and reference model (`scene_contracts.md`)

2. ⬜ Editor features (after core stable):
   - [ ] Scene Graph Inspection (depends on all core contracts)
   - [ ] Cache Diagnostics (depends on runtime cache layer)
   - [ ] Proxy & Playback Indicators (depends on runtime cache layer)
   - [ ] Multi-View Viewport (depends on `camera_3d.md`)

---

## Dependencies Summary

| Editor Feature | Depends On |
|---------------|------------|
| Multi-View Viewport | `camera_3d.md` (Phase 1-5 complete) |
| Cache Diagnostics | runtime cache layer (stats implementation) |
| Scene Graph Inspection | All: tracking pipeline, `roto_masking.md`, `camera_3d.md`, `scene_contracts.md` |
| Proxy/Playback Indicators | runtime cache layer (proxy + RAM preview) |

---

## References
- Tracking pipeline — Track bindings need to be visible in scene graph
- `roto_masking.md` — Matte links need to be inspectable
- Runtime cache layer — Cache diagnostics and playback indicators
- `camera_3d.md` — Multi-view viewport depends on camera contracts
- `interpolation_graph.md` — Graph editor is another editor-facing feature
- `scene_contracts.md` — Shared IDs, references, and serialization rules
