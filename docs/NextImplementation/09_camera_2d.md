# Camera 2D

> **Goal:** Implement 2D camera system with parallax support and layer-level transform evaluation.

## Current State

Tachyon has **2D layer transforms** implemented but lacks a dedicated 2D camera model.

**What exists:**
- ✅ `Transform2D` in `layer_spec.h` (position, rotation, scale, anchor_point)
- ✅ `include/tachyon/core/scene/evaluator/layer_evaluator.h` (evaluates layer transforms)
- ✅ `include/tachyon/renderer2d/resource/render_context.h` (render context for 2D)
- ✅ Parallax test skeleton in `tests/integration/parallax_cards_tests.cpp`

**What is missing:**
- `Camera2DSpec` — dedicated 2D camera specification
- `EvaluatedCamera2D` — evaluated camera state at a given time
- `apply_camera2d_transform()` — function to apply camera transform to layers
- `parallax_factor` per layer — depth-based parallax multiplier
- Camera layer type in `LayerSpec::type`

---

## Phase 1 — Define Camera2DSpec

**Status:** ❌ Not implemented

### Data Model

```cpp
// include/tachyon/core/spec/schema/objects/camera2d_spec.h
#pragma once

#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/core/math/algebra/vector2.h"

namespace tachyon {

struct Camera2DSpec {
    std::string id;
    std::string name{"2D Camera"};
    
    bool enabled{true};
    bool visible{true};
    
    // Zoom (equivalent to focal length in 2D space)
    AnimatedScalarSpec zoom; // default 1.0, range [0.1, 10.0]
    
    // Position in world space
    AnimatedVector2Spec position;
    
    // Rotation in degrees
    AnimatedScalarSpec rotation; // degrees
    
    // Scale
    AnimatedVector2Spec scale; // default (1,1)
    
    // Anchor point (point of interest / zoom center)
    AnimatedVector2Spec anchor_point;
    
    // Depth of field (2D approximation via blur)
    AnimatedScalarSpec focus_distance; // for parallax calculations
    
    // Viewport dimensions
    int viewport_width{1920};
    int viewport_height{1080};
};

} // namespace tachyon
```

### Files to Create

```
include/tachyon/core/spec/schema/objects/camera2d_spec.h    # NEW
```

### Files to Modify

```
include/tachyon/core/spec/schema/objects/layer_spec.h    # ADD: camera2d_spec field
include/tachyon/core/spec/schema/objects/composition_spec.h  # ADD: active_camera2d_id
src/core/spec/layer_parse_json.cpp    # ADD: Parse Camera2DSpec from JSON
```

---

## Phase 2 — Implement EvaluatedCamera2D

**Status:** ❌ Not implemented

### Data Model

```cpp
// include/tachyon/core/scene/state/evaluated_camera2d_state.h
#pragma once

#include "tachyon/core/math/algebra/matrix3x3.h"
#include "tachyon/core/math/algebra/vector2.h"

namespace tachyon {

struct EvaluatedCamera2D {
    math::Vector2 position;
    float rotation; // radians
    math::Vector2 scale;
    math::Vector2 anchor_point;
    float zoom{1.0f};
    
    // Derived transform matrix (camera space to screen space)
    math::Matrix3x3 view_matrix;
    
    // Inverse for transforming layers into camera space
    math::Matrix3x3 inverse_view_matrix;
    
    // Viewport
    int viewport_width{1920};
    int viewport_height{1080};
    
    // Compute matrices from components
    void recalculate_matrices();
};

} // namespace tachyon
```

### Files to Create

```
include/tachyon/core/scene/state/evaluated_camera2d_state.h    # NEW
src/core/scene/state/evaluated_camera2d_state.cpp              # NEW
```

---

## Phase 3 — Implement apply_camera2d_transform()

**Status:** ❌ Not implemented

### Function Signature

```cpp
// include/tachyon/core/scene/evaluator/camera2d_evaluator.h
#pragma once

#include "tachyon/core/scene/state/evaluated_camera2d_state.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/math/algebra/matrix3x3.h"

namespace tachyon {

// Apply camera transform to a layer, taking parallax into account
math::Matrix3x3 apply_camera2d_transform(
    const EvaluatedCamera2D& camera,
    const LayerSpec& layer,
    float layer_parallax_factor,
    const math::Matrix3x3& layer_world_matrix);

// Evaluate camera at a specific time
EvaluatedCamera2D evaluate_camera2d(
    const Camera2DSpec& camera_spec,
    double time_seconds);

} // namespace tachyon
```

### Implementation Logic

```cpp
// src/core/scene/evaluator/camera2d_evaluator.cpp
#include "tachyon/core/scene/evaluator/camera2d_evaluator.h"

namespace tachyon {

math::Matrix3x3 apply_camera2d_transform(
    const EvaluatedCamera2D& camera,
    const LayerSpec& layer,
    float layer_parallax_factor,
    const math::Matrix3x3& layer_world_matrix) {
    
    // Parallax: layers with higher parallax_factor are more affected by camera movement
    // A layer at the same depth as the camera (parallax_factor=1.0) moves with camera
    // A layer far away (parallax_factor=0.0) stays still
    // A layer closer than camera (parallax_factor>1.0) moves more than camera
    
    math::Vector2 parallax_offset = camera.position * (1.0f - layer_parallax_factor);
    
    // Build parallax adjustment matrix
    math::Matrix3x3 parallax_matrix = math::Matrix3x3::make_translation(parallax_offset);
    
    // Apply camera inverse view to bring layer into camera space
    // Then apply parallax adjustment
    return camera.inverse_view_matrix * parallax_matrix * layer_world_matrix;
}

EvaluatedCamera2D evaluate_camera2d(
    const Camera2DSpec& camera_spec,
    double time_seconds) {
    
    EvaluatedCamera2D result;
    result.position = camera_spec.position.evaluate(time_seconds);
    result.rotation = camera_spec.rotation.evaluate(time_seconds);
    result.scale = camera_spec.scale.evaluate(time_seconds);
    result.anchor_point = camera_spec.anchor_point.evaluate(time_seconds);
    result.zoom = camera_spec.zoom.evaluate(time_seconds);
    result.viewport_width = camera_spec.viewport_width;
    result.viewport_height = camera_spec.viewport_height;
    
    result.recalculate_matrices();
    
    return result;
}

} // namespace tachyon
```

### Files to Create

```
include/tachyon/core/scene/evaluator/camera2d_evaluator.h    # NEW
src/core/scene/evaluator/camera2d_evaluator.cpp              # NEW
```

---

## Phase 4 — Add parallax_factor to LayerSpec

**Status:** ❌ Not implemented

### Modification to LayerSpec

```cpp
// In include/tachyon/core/spec/schema/objects/layer_spec.h
struct LayerSpec {
    // ... existing fields ...
    
    // 2D Camera integration
    bool has_parallax{true};           // Does this layer respond to camera?
    float parallax_factor{1.0f};      // 0.0 = no parallax (background), 1.0 = full parallax, >1.0 = closer than camera
    
    // If empty, uses composition's active camera. If set, uses this specific camera.
    std::optional<std::string> camera2d_id;
    
    // ... rest of fields ...
};
```

### Files to Modify

```
include/tachyon/core/spec/schema/objects/layer_spec.h    # ADD: parallax fields
src/core/spec/layer_parse_json.cpp                       # ADD: Parse parallax fields
```

---

## Phase 5 — Integrate into Layer Evaluator

**Status:** ❌ Not implemented

### Modification to Layer Evaluator

The layer evaluator must:
1. Resolve active camera (from layer's `camera2d_id` or composition's `active_camera2d_id`)
2. Evaluate camera at current time
3. Apply camera transform with parallax to layer's world matrix
4. Pass transformed geometry to renderer

```cpp
// In src/core/scene/evaluator/layer_evaluator.cpp
#include "tachyon/core/scene/evaluator/camera2d_evaluator.h"

void evaluate_layer_with_camera(
    const LayerSpec& layer,
    double time,
    const std::unordered_map<std::string, Camera2DSpec>& cameras,
    const std::string& active_camera_id,
    EvaluatedLayer& result) {
    
    // Evaluate layer transform (existing code)
    math::Matrix3x3 layer_world = evaluate_layer_transform(layer, time);
    
    // Apply camera if layer has parallax
    if (layer.has_parallax && !cameras.empty()) {
        std::string camera_id = layer.camera2d_id.value_or(active_camera_id);
        auto cam_it = cameras.find(camera_id);
        if (cam_it != cameras.end()) {
            EvaluatedCamera2D camera = evaluate_camera2d(cam_it->second, time);
            layer_world = apply_camera2d_transform(
                camera, layer, layer.parallax_factor, layer_world);
        }
    }
    
    result.world_matrix = layer_world;
}
```

### Files to Modify

```
src/core/scene/evaluator/layer_evaluator.cpp    # ADD: Camera integration
include/tachyon/core/scene/evaluator/layer_evaluator.h    # ADD: Camera integration declarations
```

---

## Phase 6 — JSON Spec Example

```json
{
    "layers": [
        {
            "id": "cam_main",
            "name": "Main 2D Camera",
            "type": "camera2d",
            "camera2d_spec": {
                "position": { "x": { "value": 0 }, "y": { "value": 0 } },
                "rotation": { "value": 0 },
                "scale": { "x": { "value": 1 }, "y": { "value": 1 } },
                "anchor_point": { "x": { "value": 960 }, "y": { "value": 540 } },
                "zoom": { "value": 1.0 },
                "viewport_width": 1920,
                "viewport_height": 1080
            }
        },
        {
            "id": "bg_layer",
            "name": "Background",
            "type": "solid",
            "parallax_factor": 0.0,
            "has_parallax": true,
            "transform": {
                "position": { "x": { "value": 960 }, "y": { "value": 540 } }
            }
        },
        {
            "id": "mid_layer",
            "name": "Midground",
            "type": "solid",
            "parallax_factor": 0.5,
            "has_parallax": true
        },
        {
            "id": "fg_layer",
            "name": "Foreground",
            "type": "solid",
            "parallax_factor": 1.0,
            "has_parallax": true
        }
    ],
    "active_camera2d_id": "cam_main"
}
```

---

## Implementation Priority

1. ⬜ Phase 1: Define `Camera2DSpec` data model
2. ⬜ Phase 2: Implement `EvaluatedCamera2D` with matrix calculations
3. ⬜ Phase 3: Implement `apply_camera2d_transform()` with parallax
4. ⬜ Phase 4: Add `parallax_factor` to `LayerSpec`
5. ⬜ Phase 5: Integrate camera evaluation into layer evaluator
6. ⬜ Phase 6: Write tests for parallax behavior
7. ⬜ Phase 7: Update JSON parsing and serialization

---

## Tests to Write

```
tests/unit/core/camera2d/camera2d_spec_tests.cpp         # NEW: Camera2DSpec parsing
tests/unit/core/camera2d/camera2d_evaluator_tests.cpp    # NEW: Transform application
tests/unit/core/camera2d/parallax_tests.cpp              # NEW: Parallax factor behavior
tests/integration/parallax_cards_tests.cpp                # MODIFY: Use real camera2d
```

### Test Cases

- [ ] Camera with parallax_factor=0.0 should not move with camera
- [ ] Camera with parallax_factor=1.0 should move exactly with camera
- [ ] Camera with parallax_factor=0.5 should move at half speed
- [ ] Camera zoom should affect all layers uniformly
- [ ] Anchor point should define camera pivot point
- [ ] Nested cameras (camera2d_id override) should work per-layer
- [ ] Layers with has_parallax=false should ignore camera entirely

---

## Dependencies

- `Transform2D` (exists in `layer_spec.h`)
- `AnimatedVector2Spec`, `AnimatedScalarSpec` (exist in properties)
- `math::Matrix3x3` (may need to verify exists or implement)
- `math::Vector2` (exists)
- Layer evaluator (exists, needs modification)

---

## Notes

- Camera2D uses 3x3 matrices (2D affine transform) unlike Camera3D which uses 4x4
- Parallax is a simplification of 3D depth for 2D compositions
- Multiple cameras can exist; active camera is either per-layer or composition-wide
- Zoom is equivalent to focal length but in 2D screen space
- No near/far planes needed — 2D camera is always orthographic

