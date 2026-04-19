# Tachyon 3D Rendering — Implementation Spec

**Date**: 2026-04-19  
**Status**: Reference document — read before writing any 3D code  
**Prerequisites already in build**: FreeType, HarfBuzz, ffmpeg, nlohmann_json

---

## A. Assumptions and Conventions

### A.1 Coordinate System

- **Right-handed, Y-up**: X = right, Y = up, Z = out of screen (toward viewer)
- Camera looks down **−Z** in camera space
- AE uses a left-handed Y-down system — every AE position import must negate Y and Z
- `Vector3::forward()` = `{0, 0, -1}` (already correct in `vector3.h`)

### A.2 Matrix Layout

- **Column-major** `Matrix4x4` (already in codebase)
- Concatenation order: `M_world = M_parent * M_local` (left-multiply parent)
- `world_matrix` in `EvaluatedLayerState` stores the full object-to-world transform
- View matrix: `look_at(eye, target, up)` — already in `matrix4x4.cpp`
- Projection matrix: `perspective(fov_y, aspect, near_z, far_z)` — already in `camera_state.h`

### A.3 Units

| Field | Unit |
|---|---|
| Position X/Y/Z | AE pixels (same scale as 2D) |
| Rotation X/Y/Z | **Degrees** in JSON/spec, converted to radians at eval time |
| Zoom (camera) | AE Zoom value (pixels at distance=1 unit) |
| FOV | Derived: `fov_y = 2 * atan(comp_height / (2 * zoom))` |
| Attenuation distances | AE pixels |
| Spot cone angles | Degrees in JSON, radians internally |

### A.4 Rotation Order

AE applies rotations as: **Z then Y then X** (intrinsic, object-space).  
World matrix construction: `M_rot = Rx * Ry * Rz` (left-to-right = outer-to-inner).

### A.5 Anchor Point

- 2D anchor already in `Transform2`
- 3D anchor (`anchor_point_3d`) offsets the object origin before rotation/scale
- Formula: `M_local = T(pos) * R(orient) * T(-anchor) * S(scale)`

### A.6 Data Flow

```
SceneSpec (JSON)
    └─> Evaluator::evaluate_layer()
            └─> EvaluatedLayerState { world_matrix, world_position3, material, ... }
                    └─> FrameExecutor / EvaluatedCompositionRenderer
                            └─> project 3D layers → 2D sprites
                            └─> sort by inv_z (painter's algorithm)
                            └─> composite with depth buffer
```

### A.7 Camera Type

| AE type | Definition |
|---|---|
| One-node | Camera points at `orientation` Euler angles (no target point) |
| Two-node | Camera always points at **Point of Interest** (`poi`) regardless of position |

Default when no camera layer: orthographic approximation — camera at `{comp_w/2, comp_h/2, -1000}`, looking toward `{comp_w/2, comp_h/2, 0}`.

---

## B. MVP Scope

### B.1 IN (this implementation)

- Depth buffer (inv_z per pixel) + painter's algorithm (back-to-front sort)
- Flat shading with Blinn-Phong per 3D layer (per-layer, not per-pixel mesh shading)
- World matrix construction from position / orientation / scale / anchor_point_3d
- Camera: one-node and two-node, Zoom→FOV conversion
- Camera: depth-of-field as post-process gaussian blur (not ray-traced)
- Lights: ambient, parallel (directional), point, spot
- Spot: cone angle + penumbra, falloff types (none / smooth / linear / inverse_square)
- Light color + intensity
- Material per layer: ambient_coeff, diffuse_coeff, specular_coeff, shininess, metal_flag
- Z-sort of 3D layers before 2D compositing
- Integration into existing `EvaluatedCompositionRenderer` (not a new renderer)

### B.2 OUT (explicit non-goals for this implementation)

- Shadow maps or ray-traced shadows of any kind
- Mesh import / OBJ loading
- Image-Based Lighting (IBL), HDR environment maps
- Physically Based Rendering (PBR) beyond Blinn-Phong
- Subsurface scattering, refraction, reflection maps
- Per-pixel (fragment) shading of arbitrary geometry
- GPU acceleration
- Motion blur on 3D geometry (only 2D sub-frame sampling, already implemented)
- Volumetric effects (fog, atmosphere)
- Skinning / animation rigs

---

## C. Data Structures (precise)

### C.1 `EvaluatedLightState` — additions

Current state is missing: cone angles, falloff type, shadow flag.

```cpp
struct EvaluatedLightState {
    std::string layer_id;
    std::string type;          // "ambient" | "parallel" | "point" | "spot"
    math::Vector3 position{math::Vector3::zero()};
    math::Vector3 direction{0.0f, 0.0f, -1.0f};  // normalized, world space
    ColorSpec color{255, 255, 255, 255};
    float intensity{1.0f};

    // Attenuation (point + spot only)
    float attenuation_near{0.0f};   // distance where full intensity starts
    float attenuation_far{1000.0f}; // distance where intensity reaches 0

    // Spot cone (spot only)
    float cone_angle_rad{0.698f};       // ~40 deg half-angle (inner cone)
    float cone_feather_rad{0.175f};     // ~10 deg penumbra width

    // Falloff shape
    // "none" | "smooth" | "linear" | "inverse_square"
    std::string falloff_type{"smooth"};

    // Shadows — reserved for future, always false in MVP
    bool casts_shadows{false};
};
```

### C.2 `EvaluatedCameraState` — additions

```cpp
struct EvaluatedCameraState {
    bool available{false};
    std::string layer_id;
    math::Vector3 position{math::Vector3::zero()};
    math::Vector3 point_of_interest{0.0f, 0.0f, 0.0f}; // two-node target
    bool is_two_node{true};  // AE default

    // Depth of Field
    bool dof_enabled{false};
    float focus_distance{1000.0f};  // AE pixels along camera axis
    float aperture{1.0f};           // controls blur radius
    float blur_level{1.0f};         // 0..1 scale on blur radius

    camera::CameraState camera;     // fov_y_rad, aspect, near_z, far_z + matrices
};
```

### C.3 `EvaluatedLayerState` — additions

```cpp
// Add to EvaluatedLayerState (below world_matrix):

math::Vector3 orientation_xyz_deg{math::Vector3::zero()}; // Euler X/Y/Z degrees
math::Vector3 anchor_point_3d{math::Vector3::zero()};     // object-space anchor
math::Vector3 scale_3d{1.0f, 1.0f, 1.0f};                // per-axis scale

struct MaterialOptions {
    float ambient_coeff{0.5f};
    float diffuse_coeff{0.5f};
    float specular_coeff{0.0f};
    float shininess{15.0f};
    bool  metal{false};  // metal: specular color = layer color (not white)
};
MaterialOptions material;

// Computed by evaluator, used by renderer
math::Vector3 world_normal{0.0f, 0.0f, 1.0f}; // normal of the layer plane in world space
```

### C.4 `DepthBuffer`

New type, lives in `include/tachyon/renderer2d/depth_buffer.h`:

```cpp
namespace tachyon::renderer2d {

class DepthBuffer {
public:
    DepthBuffer() = default;
    DepthBuffer(int width, int height);

    void clear();  // fills with 0 (nothing in front)

    // Returns true if inv_z passes (i.e. is in front of stored value).
    // If passes, writes inv_z to buffer.
    bool test_and_write(int x, int y, float inv_z);

    float get(int x, int y) const;

private:
    int width_{0};
    int height_{0};
    std::vector<float> data_;  // inv_z values, 0 = empty
};

} // namespace tachyon::renderer2d
```

**inv_z convention**: `inv_z = 1.0f / z_camera` where `z_camera > 0` for visible objects.  
Larger inv_z → closer → wins depth test.  
At near plane (z = near_z): `inv_z_max = 1.0f / near_z`.

---

## D. World Matrix Construction

### D.1 Formula

```
M_world = M_parent_world * T(position) * Rx(ox) * Ry(oy) * Rz(oz) * T(-anchor) * S(scale)
```

where `ox, oy, oz` are orientation angles in **radians** (converted from degrees at eval time).

### D.2 `build_layer_world_matrix` (in `src/scene/evaluator.cpp`)

```cpp
math::Matrix4x4 build_layer_world_matrix(
    const math::Matrix4x4& parent_world,
    const math::Vector3& position,
    const math::Vector3& orientation_deg,
    const math::Vector3& scale,
    const math::Vector3& anchor_point)
{
    using M = math::Matrix4x4;
    const float deg2rad = math::kPi / 180.0f;

    auto Rx = M::rotation_x(orientation_deg.x * deg2rad);
    auto Ry = M::rotation_y(orientation_deg.y * deg2rad);
    auto Rz = M::rotation_z(orientation_deg.z * deg2rad);
    auto R  = Rx * Ry * Rz;

    auto T_pos    = M::translation(position.x, position.y, position.z);
    auto T_anchor = M::translation(-anchor_point.x, -anchor_point.y, -anchor_point.z);
    auto S        = M::scaling(scale.x, scale.y, scale.z);

    auto M_local = T_pos * R * T_anchor * S;
    return parent_world * M_local;
}
```

### D.3 World Normal

For a flat layer (all 3D layers are billboards or planes), the surface normal in object space is `{0, 0, 1}`.  
World normal = `normalize(M_rot * {0,0,1})` where `M_rot = Rx*Ry*Rz` (upper-left 3×3 of world matrix).

Extract from `world_matrix`:

```cpp
math::Vector3 extract_world_normal(const math::Matrix4x4& m) {
    // Column 2 of rotation part (Z column)
    math::Vector3 n{m.m[2][0], m.m[2][1], m.m[2][2]};
    return n.normalized();
}
```

---

## E. Camera Construction

### E.1 Zoom → FOV

```cpp
float zoom_to_fov_y(float zoom, float comp_height) {
    // AE: zoom = distance at which 1 unit subtends exactly 1 pixel
    // half_height_at_zoom = comp_height / 2 pixels at distance = zoom
    return 2.0f * std::atan(comp_height * 0.5f / zoom);
}
```

Default zoom when not set: `comp_height / (2 * tan(30°))` ≈ AE default.

### E.2 View Matrix

```cpp
math::Matrix4x4 build_view_matrix(
    const math::Vector3& eye,
    const math::Vector3& poi,      // two-node target
    bool is_two_node,
    const math::Vector3& orientation_deg)  // one-node euler
{
    if (is_two_node) {
        math::Vector3 up{0.0f, 1.0f, 0.0f};
        // Handle degenerate: eye == poi → return identity view
        if ((poi - eye).length_sq() < 1e-8f) return math::Matrix4x4::identity();
        return math::Matrix4x4::look_at(eye, poi, up);
    } else {
        // One-node: apply orientation Euler to camera-space forward
        const float d2r = math::kPi / 180.0f;
        auto Rx = math::Matrix4x4::rotation_x(orientation_deg.x * d2r);
        auto Ry = math::Matrix4x4::rotation_y(orientation_deg.y * d2r);
        auto Rz = math::Matrix4x4::rotation_z(orientation_deg.z * d2r);
        math::Vector3 fwd = (Rx * Ry * Rz).transform_direction(math::Vector3::forward());
        math::Vector3 target = eye + fwd;
        math::Vector3 up{0.0f, 1.0f, 0.0f};
        return math::Matrix4x4::look_at(eye, target, up);
    }
}
```

---

## F. Lighting Model

### F.1 Light Contribution per Layer

```cpp
struct LightResult { float r, g, b; };  // linear, premultiplied factor

LightResult compute_light_contribution(
    const EvaluatedLightState& light,
    const math::Vector3& layer_world_pos,
    const math::Vector3& layer_world_normal,
    const math::Vector3& view_dir_normalized,   // toward camera
    const EvaluatedLayerState::MaterialOptions& mat)
{
    float light_r = light.color.r / 255.0f * light.intensity;
    float light_g = light.color.g / 255.0f * light.intensity;
    float light_b = light.color.b / 255.0f * light.intensity;

    if (light.type == "ambient") {
        return { light_r * mat.ambient_coeff,
                 light_g * mat.ambient_coeff,
                 light_b * mat.ambient_coeff };
    }

    // Direction to light
    math::Vector3 L;
    float atten = 1.0f;

    if (light.type == "parallel") {
        L = (-light.direction).normalized();
    } else {
        // point or spot
        math::Vector3 to_light = light.position - layer_world_pos;
        float dist = to_light.length();
        L = (dist > 1e-6f) ? to_light * (1.0f / dist) : math::Vector3{0,1,0};
        atten = compute_falloff(dist, light);
    }

    // Spot cone
    if (light.type == "spot") {
        float cos_angle = math::Vector3::dot((-light.direction).normalized(), L);
        float inner_cos = std::cos(light.cone_angle_rad);
        float outer_cos = std::cos(light.cone_angle_rad + light.cone_feather_rad);
        if (cos_angle < outer_cos) return {0, 0, 0};
        if (cos_angle < inner_cos) {
            float t = (cos_angle - outer_cos) / std::max(inner_cos - outer_cos, 1e-6f);
            atten *= t * t;  // smooth falloff in penumbra
        }
    }

    // Diffuse
    float NdotL = std::max(0.0f, math::Vector3::dot(layer_world_normal, L));
    float diffuse = mat.diffuse_coeff * NdotL;

    // Specular (Blinn-Phong)
    float specular = 0.0f;
    if (mat.specular_coeff > 0.0f && NdotL > 0.0f) {
        math::Vector3 H = (L + view_dir_normalized).normalized();
        float NdotH = std::max(0.0f, math::Vector3::dot(layer_world_normal, H));
        specular = mat.specular_coeff * std::pow(NdotH, mat.shininess);
    }

    // Metal: specular color tinted by layer color (handled in caller)
    float total = (diffuse + specular) * atten;
    return { light_r * total, light_g * total, light_b * total };
}
```

### F.2 Falloff Computation

```cpp
float compute_falloff(float dist, const EvaluatedLightState& light) {
    float near = light.attenuation_near;
    float far  = light.attenuation_far;

    if (dist <= near) return 1.0f;
    if (dist >= far)  return 0.0f;
    if (far <= near)  return 0.0f;  // degenerate: treat as off at far

    float t = (dist - near) / (far - near);  // 0..1

    if (light.falloff_type == "none")           return 1.0f;
    if (light.falloff_type == "linear")         return 1.0f - t;
    if (light.falloff_type == "inverse_square") return 1.0f / (1.0f + t * t * 10.0f);
    // "smooth" (default)
    return (1.0f - t) * (1.0f - t);
}
```

### F.3 Applying Lighting to a Layer's Sprite

After projecting a 3D layer to its 2D billboard position, apply a **scalar tint** to its rendered surface:

```cpp
// Accumulate all lights
float acc_r = 0, acc_g = 0, acc_b = 0;
for (const auto& light : composition.lights) {
    auto contrib = compute_light_contribution(
        light, layer.world_position3, layer.world_normal,
        view_dir, layer.material);
    acc_r += contrib.r;
    acc_g += contrib.g;
    acc_b += contrib.b;
}
// Clamp to [0, 1]
acc_r = std::min(1.0f, acc_r);
acc_g = std::min(1.0f, acc_g);
acc_b = std::min(1.0f, acc_b);

// Apply to every pixel of the layer surface (premultiplied-safe):
// out.r = pixel.r * acc_r, etc. (alpha unchanged)
```

Metal flag: when `mat.metal == true`, the specular highlight color = layer's base color, not white. Implement by passing layer color to `compute_light_contribution` and multiplying specular term by `layer_color / 255.0f`.

---

## G. Depth of Field (Post-Process)

DOF is applied after all 3D layers are composited onto the frame but before 2D layers:

### G.1 Blur Radius per Layer

```cpp
float dof_blur_radius(
    float z_world,          // distance of layer center from camera along camera axis
    const EvaluatedCameraState& cam)
{
    if (!cam.dof_enabled) return 0.0f;

    float focus_dist = cam.focus_distance;
    float defocus = std::abs(z_world - focus_dist);

    // AE-style: aperture controls circle of confusion
    // blur_radius = aperture * (defocus / focus_dist) * blur_level * scale_factor
    const float scale = 0.5f;  // empirical, matches AE visual at default settings
    float r = cam.aperture * (defocus / std::max(focus_dist, 1.0f))
              * cam.blur_level * scale;

    return std::min(r, 50.0f);  // cap at 50px to prevent runaway
}
```

### G.2 Application

- Per 3D layer: compute blur radius from its `world_position3` projected onto camera axis
- Apply separable gaussian blur to the layer's rendered surface before compositing
- Layers at exactly `focus_distance` get radius ≈ 0 (no blur)
- Bokeh shape: circular approximation via two-pass separable gaussian

---

## H. Integration into EvaluatedCompositionRenderer

### H.1 Rendering Order

1. Collect all 3D layers (`layer.is_3d == true`)
2. Project each to screen position (MVP transform)
3. Compute inv_z for each
4. Sort 3D layers back-to-front by inv_z ascending (farthest first)
5. Render each 3D layer as a 2D sprite at its projected position (with lighting tint + DOF blur)
6. Write to depth buffer
7. Continue normal 2D layer stack on top of 3D result

### H.2 Projection

```cpp
struct ProjectedLayer {
    std::size_t layer_index;
    float screen_x, screen_y;   // center in comp pixels
    float scale_factor;          // perspective scale relative to identity
    float inv_z;
    float z_camera;              // for DOF
};

ProjectedLayer project_3d_layer(
    const EvaluatedLayerState& layer,
    const math::Matrix4x4& view,
    const math::Matrix4x4& proj,
    float comp_width, float comp_height)
{
    // Transform layer center (world_position3) to clip space
    math::Vector4 world_pos{layer.world_position3.x,
                            layer.world_position3.y,
                            layer.world_position3.z, 1.0f};
    math::Vector4 clip = proj * view * world_pos;

    // Perspective divide
    float w = clip.w;
    if (std::abs(w) < 1e-6f) return {}; // behind camera
    float ndc_x = clip.x / w;
    float ndc_y = clip.y / w;

    // NDC [-1,1] to screen [0, comp_w] x [0, comp_h]
    float sx = (ndc_x + 1.0f) * 0.5f * comp_width;
    float sy = (1.0f - ndc_y) * 0.5f * comp_height;  // Y flip (NDC up → screen down)

    float z_cam = (view * world_pos).z;  // negative for visible (cam looks -Z)
    float inv_z = (z_cam < 0.0f) ? -1.0f / z_cam : 0.0f;

    // Scale factor: ratio of perspective zoom at this depth vs identity
    float scale_f = (z_cam < 0.0f) ? std::abs(1.0f / z_cam) : 0.0f;

    return { layer.layer_index, sx, sy, scale_f, inv_z, std::abs(z_cam) };
}
```

---

## I. Edge Cases

| Scenario | Handling |
|---|---|
| `zoom <= 0` | Clamp to 1.0 with a warning log; never produce NaN FOV |
| `attenuation_far <= attenuation_near` | Treat as zero radius light (degenerate), return `atten = 0` |
| `attenuation_far == attenuation_near` | Same as above (avoid division by zero) |
| Layer behind camera (`z_cam >= 0`) | Skip projection, do not render |
| Layer at exactly camera position | Skip (`inv_z` → ∞) |
| World matrix determinant ≈ 0 | Skip layer, log warning — degenerate transform |
| `poi == camera position` (two-node degenerate) | Return `Matrix4x4::identity()` for view, log warning |
| No camera layer in composition | Use default: pos={cx, cy, -1000}, poi={cx, cy, 0} (orthographic-like) |
| All lights missing | Layer receives zero lighting → black; at least one ambient recommended |
| `focus_distance <= 0` | Clamp to 1.0 |
| Spot with `cone_angle_rad <= 0` | Treat as point light |
| `shininess <= 0` | Clamp to 0.1 to avoid pow(x, 0) = 1 artifact |
| `scale_3d component == 0` | Skip layer (zero-area), log warning |

---

## J. Test Plan

### J.1 Unit Tests (`tests/unit/renderer2d/3d_*`)

| Test | What to verify |
|---|---|
| `world_matrix_identity` | Layer at origin, no rotation, no scale → identity matrix |
| `world_matrix_translation` | Layer at (100, 200, 300) → world_position3 matches |
| `world_matrix_rotation_x` | 90° X rotation → Y axis becomes Z axis |
| `world_matrix_anchor` | Anchor at (50,0,0) + pos at (100,0,0) → center at (50,0,0) |
| `camera_zoom_to_fov` | zoom=comp_h/2 → fov_y=90°; zoom→∞ → fov_y→0 |
| `camera_two_node_view` | Camera at (0,0,-500) looking at origin → correct view matrix |
| `camera_one_node_view` | 0° orientation → same as looking at origin |
| `falloff_smooth` | dist=near → 1.0; dist=far → 0.0; dist=midpoint → 0.25 |
| `falloff_degenerate` | far <= near → always 0 |
| `spot_outside_cone` | Point outside outer cone → contribution = 0 |
| `spot_inside_inner` | Point inside inner cone → full attenuation (no penumbra reduction) |
| `ambient_light` | Ambient only → acc = ambient_coeff * intensity |
| `parallel_light_ndotl` | Light normal-on → full diffuse; light 90° → zero |
| `depth_buffer_test_and_write` | Two layers at same pixel, closer wins |
| `depth_buffer_clear` | After clear, all inv_z = 0 |
| `dof_blur_radius_at_focus` | z == focus_distance → radius ≈ 0 |
| `dof_blur_radius_beyond_focus` | z > focus_distance → radius increases with aperture |

### J.2 Visual Acceptance Tests (`tests/visual/3d_*`)

Each test renders a known scene and compares against a reference PNG (pixel-diff ≤ 2%):

| Scene | Validates |
|---|---|
| Single solid layer at z=0 under ambient-only → should match 2D render | Lighting math does not destroy flat 2D layers |
| Two solid layers at different Z, one occluding the other | Depth sort (painter's algorithm) |
| Solid layer at z=−500 with parallel light at 45° | Correct diffuse shading on a billboard |
| Point light falloff: layer at near/mid/far distance | Attenuation curve shapes |
| Spot cone test: layer in inner / penumbra / outer zones | Cone and penumbra boundaries |
| Two-node camera pointing at offset target | View matrix, projection, Y-flip |
| DOF: three layers at near/focus/far — middle sharp | DOF blur increases with defocus |

---

## K. Performance Budget

Target: 1080p frame, 4 active 3D layers, 4 lights, single thread.

| Operation | Budget |
|---|---|
| World matrix construction (all layers) | < 0.5 ms |
| Light accumulation (4 lights × 4 layers) | < 0.2 ms |
| DOF gaussian blur (50px radius, 1 layer) | < 10 ms |
| Depth sort | < 0.1 ms |
| Total 3D overhead per frame (excluding rasterization) | < 15 ms |

Rasterization cost is shared with the existing 2D pipeline and not counted here.

---

## L. Explicit Non-Goals (this implementation)

- Shadows (reserved for future — field `casts_shadows` exists but is always `false`)
- Reflections on surfaces
- Environment maps / IBL
- Refraction
- Volumetric fog
- Per-pixel fragment shading of geometry (all shading is per-layer, not per-texel)
- Mesh import from OBJ/FBX/glTF
- Skeletal animation
- Motion blur on 3D world positions (only 2D sub-frame sampling is supported)
- GPU/Vulkan/OpenGL acceleration
- Render-to-texture for 3D layers

---

## M. File Breakdown

| File | Change |
|---|---|
| `include/tachyon/scene/evaluated_state.h` | Add `orientation_xyz_deg`, `anchor_point_3d`, `scale_3d`, `MaterialOptions`, `world_normal` to `EvaluatedLayerState`; add DOF + POI + `is_two_node` to `EvaluatedCameraState`; add cone/falloff fields to `EvaluatedLightState` |
| `src/scene/evaluator.cpp` | Implement `build_layer_world_matrix`, compute `world_normal`, populate new fields from `LayerSpec` |
| `include/tachyon/renderer2d/depth_buffer.h` | New: `DepthBuffer` class |
| `src/renderer2d/depth_buffer.cpp` | New: `DepthBuffer` implementation |
| `src/renderer2d/evaluated_composition_renderer.cpp` | Add 3D layer projection, z-sort, depth buffer, lighting tint, DOF blur pass |
| `src/CMakeLists.txt` | Add `renderer2d/depth_buffer.cpp` |
| `tests/unit/renderer2d/3d_world_matrix_tests.cpp` | New: world matrix unit tests |
| `tests/unit/renderer2d/3d_lighting_tests.cpp` | New: lighting/falloff unit tests |
| `tests/unit/renderer2d/3d_depth_buffer_tests.cpp` | New: depth buffer unit tests |
| `tests/CMakeLists.txt` | Add new test files |

---

## N. AE JSON Field Mapping

Fields expected in `LayerSpec` for 3D layers:

```json
{
  "is_3d": true,
  "position": [x, y, z],
  "orientation": [rx_deg, ry_deg, rz_deg],
  "scale": [sx_percent, sy_percent, sz_percent],
  "anchor_point": [ax, ay, az],
  "material_options": {
    "ambient": 0.5,
    "diffuse": 0.5,
    "specular": 0.0,
    "shininess": 15.0,
    "metal": false
  }
}
```

Light layer fields:

```json
{
  "type": "spot",
  "position": [x, y, z],
  "direction": [dx, dy, dz],
  "color": [r, g, b],
  "intensity": 1.0,
  "attenuation_near": 0.0,
  "attenuation_far": 1000.0,
  "cone_angle": 40.0,
  "cone_feather": 10.0,
  "falloff": "smooth"
}
```

Camera layer fields:

```json
{
  "type": "camera",
  "position": [x, y, z],
  "point_of_interest": [px, py, pz],
  "is_two_node": true,
  "zoom": 1000.0,
  "dof_enabled": false,
  "focus_distance": 1000.0,
  "aperture": 1.0,
  "blur_level": 1.0
}
```

---

*End of spec. All implementation must conform to this document. Any deviation requires updating the spec first.*
