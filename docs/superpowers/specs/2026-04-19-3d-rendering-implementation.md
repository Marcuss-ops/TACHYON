# Tachyon 3D Rendering — Spec di Implementazione

**Data**: 2026-04-19  
**Target**: implementazione completa del pipeline 3D CPU AE-style  
**Prerequisiti di build già presenti**: FreeType, HarfBuzz, ffmpeg, nlohmann_json

---

## Stato attuale del codebase (baseline)

Già esistente e funzionante:
- `EvaluatedLightState`: type, position, direction, color, intensity, attenuation — **mancano** cone, falloff, shadows
- `EvaluatedCameraState`: position, `CameraState` (fov, aspect, near/far, view matrix, projection matrix, `project_point`)
- `EvaluatedLayerState`: `is_3d`, `world_position3`, `world_matrix` — **mancano** orientation XYZ, anchor_point_3d, material options
- `PerspectiveRasterizer`: disegna quad planari con perspective-correct UV
- `DepthBuffer`: stub in `surface_rgba.cpp` — da completare e integrare nel rasterizer
- `EvaluatedCompositionState`: ha già `std::vector<EvaluatedLightState> lights`

---

## Librerie esterne necessarie

### Phase 1 — Nessuna libreria nuova

Il pipeline 3D base (depth buffer, luci, DOF, material) è matematica pura. Nessuna dipendenza aggiuntiva.

### Phase 2 — Import modelli (opzionale, post-MVP)

| Libreria | Uso | Come ottenerla |
|---------|-----|----------------|
| **tinygltf** | Import GLTF/GLB con animazioni | Header-only, `FetchContent_Declare` da `https://github.com/syoyo/tinygltf.git` |
| **cgltf** | Alternativa più leggera a tinygltf | Single-header, copia diretta in `third_party/` |
| **stb_image** | Caricamento HDRI per IBL | Già probabilmente disponibile via ffmpeg; altrimenti single-header |

Per Phase 1 (domani) non serve nulla di nuovo.

---

## Phase 1 — Fondamenta 3D (1 giornata)

### 1.1 Strutture dati da estendere

#### `EvaluatedLayerState` — aggiungere in `evaluated_state.h`

```cpp
// 3D Transform completo
math::Vector3 orientation_xyz{math::Vector3::zero()};   // Euler XYZ in gradi
math::Vector3 anchor_point_3d{math::Vector3::zero()};

// Material Options (premere AA in AE)
struct MaterialOptions {
    bool  casts_shadows{true};
    bool  accepts_shadows{true};
    bool  accepts_lights{true};
    float ambient{100.0f};         // 0-100%
    float diffuse{100.0f};         // 0-100%
    float specular_intensity{50.0f};
    float specular_shininess{5.0f};
    float metal{0.0f};             // 0% = highlight colore luce, 100% = colore layer
    float light_transmission{0.0f};// 0-100%, per vetri
};
MaterialOptions material;
```

#### `EvaluatedLightState` — aggiungere in `evaluated_state.h`

```cpp
struct EvaluatedLightState {
    std::string layer_id;
    std::string type; // "ambient" | "parallel" | "point" | "spot"
    math::Vector3 position{math::Vector3::zero()};
    math::Vector3 direction{0.0f, 0.0f, -1.0f}; // normalizzato

    ColorSpec color{255, 255, 255, 255};
    float intensity{100.0f};      // 0-100+%, può essere negativa

    // Spot
    float cone_angle{90.0f};      // gradi
    float cone_feather{50.0f};    // 0-100%

    // Falloff
    std::string falloff{"none"};  // "none" | "smooth" | "inverse_square_clamped"
    float falloff_radius{0.0f};
    float falloff_distance{500.0f};

    // Ombre
    bool  casts_shadows{false};
    float shadow_darkness{100.0f};
    float shadow_diffusion{0.0f};
};
```

#### `EvaluatedCameraState` — estendere

```cpp
struct EvaluatedCameraState {
    bool available{false};
    std::string layer_id;

    // Tipo camera
    std::string camera_type{"one_node"}; // "one_node" | "two_node"
    math::Vector3 position{math::Vector3::zero()};
    math::Vector3 point_of_interest{0.0f, 0.0f, 0.0f}; // solo two-node

    // Ottica
    float zoom{877.0f};           // distanza lente-piano immagine (px a 100%)
    float focal_length{50.0f};    // mm
    float film_size{36.0f};       // mm, default 35mm full frame
    float angle_of_view{39.6f};   // gradi, calcolato da focal_length/film_size

    // Depth of Field
    bool  dof_enabled{false};
    float focus_distance{1000.0f};
    float aperture{4.0f};         // f-stop
    float blur_level{100.0f};     // % di intensità bokeh

    camera::CameraState camera;   // view + projection matrix calcolati
};
```

---

### 1.2 Depth Buffer

**File**: `include/tachyon/renderer2d/depth_buffer.h` + `src/renderer2d/depth_buffer.cpp`

```cpp
// depth_buffer.h
struct DepthBuffer {
    std::vector<float> inv_z;   // 1/z per pixel (perspective-correct)
    std::uint32_t width{0};
    std::uint32_t height{0};

    DepthBuffer() = default;
    DepthBuffer(std::uint32_t w, std::uint32_t h);

    void clear();
    // Ritorna true se il frammento è visibile e aggiorna il buffer
    bool test_and_write(int x, int y, float inv_z_val);
};
```

```cpp
// depth_buffer.cpp
DepthBuffer::DepthBuffer(std::uint32_t w, std::uint32_t h)
    : inv_z(static_cast<std::size_t>(w) * h, 0.0f), width(w), height(h) {}

void DepthBuffer::clear() {
    std::fill(inv_z.begin(), inv_z.end(), 0.0f);
}

bool DepthBuffer::test_and_write(int x, int y, float inv_z_val) {
    if (x < 0 || y < 0 || x >= static_cast<int>(width) || y >= static_cast<int>(height))
        return false;
    const std::size_t idx = static_cast<std::size_t>(y) * width + x;
    if (inv_z_val <= inv_z[idx]) return false;
    inv_z[idx] = inv_z_val;
    return true;
}
```

**Integrazione nel PerspectiveRasterizer**: ogni frammento chiama `depth_buffer->test_and_write(x, y, frag_inv_z)`. Se false, skip. Questo sostituisce il depth sort by camera distance (che fallisce con layer intersecanti).

**Dove vive**: `DepthBuffer` viene allocato in `render_composition_recursive` per frame e passato nel `RenderContext` (non globale — già nel contesto per thread-safety).

---

### 1.3 Calcolo matrice world per layer 3D

L'attuale `world_matrix` nel layer è `Matrix4x4::identity()`. Va calcolata dall'evaluator usando position, orientation, anchor_point.

**In `evaluator.cpp`**, quando `layer.is_3d`:

```cpp
math::Matrix4x4 build_layer_world_matrix(
    const math::Vector3& position,
    const math::Vector3& orientation_xyz_deg,  // Euler XYZ
    const math::Vector3& anchor_point,
    const math::Vector3& scale) {

    // AE order: TRS con pivot all'anchor point
    const float rx = orientation_xyz_deg.x * M_PI / 180.0f;
    const float ry = orientation_xyz_deg.y * M_PI / 180.0f;
    const float rz = orientation_xyz_deg.z * M_PI / 180.0f;

    const auto T_pos    = Matrix4x4::translation(position);
    const auto R_x      = Matrix4x4::rotation_x(rx);
    const auto R_y      = Matrix4x4::rotation_y(ry);
    const auto R_z      = Matrix4x4::rotation_z(rz);
    const auto S        = Matrix4x4::scale(scale);
    const auto T_anchor = Matrix4x4::translation(-anchor_point);

    // AE TRS: Position * Rz * Ry * Rx * Scale * (-AnchorPoint)
    return T_pos * R_z * R_y * R_x * S * T_anchor;
}
```

---

### 1.4 Camera — calcolo view matrix

**One-node**: la camera usa `transform` direttamente (posizione + orientazione esplicita).

**Two-node**: la camera punta sempre verso `point_of_interest`.

```cpp
math::Matrix4x4 build_two_node_view_matrix(
    const math::Vector3& position,
    const math::Vector3& point_of_interest,
    const math::Vector3& up = {0, 1, 0}) {

    // LookAt standard
    const math::Vector3 forward = (point_of_interest - position).normalized();
    const math::Vector3 right   = up.cross(forward).normalized();
    const math::Vector3 true_up = forward.cross(right);
    // costruire la matrice view 4x4 da right, true_up, forward, position
    return math::Matrix4x4::look_at(position, point_of_interest, up);
}
```

**Zoom → FOV**: in AE, `zoom` è la distanza in pixel dal film al lente a 100% scala. La conversione in FOV:

```cpp
float zoom_to_fov_y(float zoom, float comp_height) {
    // angle_of_view = 2 * atan(film_size / (2 * focal_length))
    // In AE: zoom è espresso in pixel della composizione
    return 2.0f * std::atan(comp_height / (2.0f * zoom));
}
```

Aggiornare `CameraState::fov_y_rad` dall'evaluator usando zoom e comp_height.

---

### 1.5 Illuminazione per-layer (flat shading)

**In `evaluated_composition_renderer.cpp`**, dopo aver proiettato i corner del layer ma prima del rasterize, calcolare il colore netto del layer considerando le luci.

```cpp
renderer2d::Color apply_lighting_to_layer(
    renderer2d::Color base_color,
    const scene::EvaluatedLayerState& layer,
    const std::vector<scene::EvaluatedLightState>& lights,
    const math::Vector3& layer_world_pos,
    const math::Vector3& layer_normal)  // normale calcolata dalla world_matrix
{
    if (!layer.material.accepts_lights || lights.empty()) {
        return base_color;
    }

    const float base_r = static_cast<float>(base_color.r) / 255.0f;
    const float base_g = static_cast<float>(base_color.g) / 255.0f;
    const float base_b = static_cast<float>(base_color.b) / 255.0f;

    float r = base_r * layer.material.ambient / 100.0f * 0.1f; // ambient floor
    float g = base_g * layer.material.ambient / 100.0f * 0.1f;
    float b = base_b * layer.material.ambient / 100.0f * 0.1f;

    for (const auto& light : lights) {
        const float light_r = static_cast<float>(light.color.r) / 255.0f;
        const float light_g = static_cast<float>(light.color.g) / 255.0f;
        const float light_b = static_cast<float>(light.color.b) / 255.0f;
        const float intensity = light.intensity / 100.0f;

        if (light.type == "ambient") {
            r += base_r * light_r * intensity * layer.material.diffuse / 100.0f;
            g += base_g * light_g * intensity * layer.material.diffuse / 100.0f;
            b += base_b * light_b * intensity * layer.material.diffuse / 100.0f;
            continue;
        }

        math::Vector3 light_dir;
        float attenuation = 1.0f;

        if (light.type == "parallel") {
            light_dir = (-light.direction).normalized();
        } else if (light.type == "point" || light.type == "spot") {
            light_dir = (light.position - layer_world_pos).normalized();
            const float dist = (light.position - layer_world_pos).length();
            attenuation = compute_falloff(light, dist);
        }

        const float ndotl = std::max(0.0f, layer_normal.dot(light_dir));

        // Diffuse
        const float diff = ndotl * intensity * attenuation * layer.material.diffuse / 100.0f;
        r += base_r * light_r * diff;
        g += base_g * light_g * diff;
        b += base_b * light_b * diff;

        // Specular (Blinn-Phong)
        if (layer.material.specular_intensity > 0.0f) {
            // half_vec richiede la direzione verso la camera
            // passare camera_pos nel context per questo calcolo
            const float spec_power = layer.material.specular_shininess * 10.0f;
            // ... calcolo highlight speculare
            const float metal_factor = layer.material.metal / 100.0f;
            // metal=0: highlight bianco (colore luce)
            // metal=1: highlight colore layer
            // ...
        }

        // Spot cone falloff
        if (light.type == "spot") {
            const float cone_cos     = std::cos(light.cone_angle * 0.5f * M_PI / 180.0f);
            const float feather_cos  = std::cos((light.cone_angle * 0.5f + light.cone_feather * 0.5f) * M_PI / 180.0f);
            const float spot_dot     = (-light_dir).dot(light.direction.normalized());
            const float spot_factor  = std::clamp((spot_dot - feather_cos) / (cone_cos - feather_cos + 1e-6f), 0.0f, 1.0f);
            r *= spot_factor; g *= spot_factor; b *= spot_factor;
        }
    }

    return renderer2d::Color{
        static_cast<std::uint8_t>(std::clamp(std::lround(r * 255.0f), 0L, 255L)),
        static_cast<std::uint8_t>(std::clamp(std::lround(g * 255.0f), 0L, 255L)),
        static_cast<std::uint8_t>(std::clamp(std::lround(b * 255.0f), 0L, 255L)),
        base_color.a
    };
}

float compute_falloff(const scene::EvaluatedLightState& light, float dist) {
    if (light.falloff == "none") return 1.0f;
    if (dist <= light.falloff_radius) return 1.0f;
    const float t = (dist - light.falloff_radius) / (light.falloff_distance - light.falloff_radius + 1e-6f);
    if (light.falloff == "smooth") return std::max(0.0f, 1.0f - t);
    if (light.falloff == "inverse_square_clamped") return 1.0f / (1.0f + t * t);
    return 1.0f;
}
```

**Normale del layer**: estratta dalla colonna Z della `world_matrix` dopo rotazione.

```cpp
math::Vector3 layer_normal_from_world_matrix(const math::Matrix4x4& world) {
    // La colonna Z della rotazione è la normale del piano del layer
    return math::Vector3{world.m[2], world.m[6], world.m[10]}.normalized();
}
```

---

### 1.6 Depth of Field — post-process Gaussian blur

Il DOF in AE è un blur gaussiano applicato ai layer fuori fuoco. L'implementazione CPU più semplice e corretta:

1. Renderizzare tutti i layer normalmente (con depth buffer).
2. Per ogni pixel, calcolare `depth = 1.0f / depth_buffer.inv_z[pixel]`.
3. `circle_of_confusion = aperture * abs(depth - focus_distance) / depth`.
4. Applicare blur gaussiano con `sigma = circle_of_confusion * blur_level / 100.0f`.

Per CPU, un blur gaussiano separable (pass orizzontale + verticale) è O(N·W) invece di O(N·W²).

```cpp
renderer2d::Framebuffer apply_dof_blur(
    const renderer2d::Framebuffer& input,
    const DepthBuffer& depth_buf,
    const scene::EvaluatedCameraState& cam)
{
    if (!cam.dof_enabled) return input;

    // Calcola per ogni pixel il CoC (Circle of Confusion)
    const std::uint32_t w = input.width();
    const std::uint32_t h = input.height();
    std::vector<float> coc(w * h, 0.0f);

    for (std::uint32_t y = 0; y < h; ++y) {
        for (std::uint32_t x = 0; x < w; ++x) {
            const std::size_t idx = y * w + x;
            const float inv_z = depth_buf.inv_z[idx];
            if (inv_z <= 0.0f) continue;
            const float depth = 1.0f / inv_z;
            const float dof_scale = cam.blur_level / 100.0f;
            coc[idx] = std::abs(depth - cam.focus_distance) /
                       (cam.focus_distance + 1e-6f) *
                       cam.aperture * dof_scale * 50.0f; // 50 = fattore di scala empirico
        }
    }

    // Gaussian blur separable con sigma variabile per pixel
    // Semplificazione: blur uniforme basato sulla mediana CoC della regione
    // (per un DOF fisicamente corretto servono scatter-as-gather o bokeh filter)
    return gaussian_blur_variable(input, coc);
}
```

Per il MVP: blur uniforme basato su `max(coc)` della composizione — visivamente accettabile, non fisicamente preciso. Il DOF fisicamente corretto è Phase 2.

---

## Phase 2 — Shadow Maps (post-MVP)

Le ombre richiedono un secondo render pass dalla prospettiva della luce.

### Solo per luce "parallel" (proiezione ortografica, più semplice)

```
Pass 1 — Shadow Map:
  Proiezione ortografica dalla direzione della luce
  Per ogni layer 3D: rasterizza nella shadow_map solo il depth
  Output: shadow_map texture (float buffer 1 canale)

Pass 2 — Render principale:
  Per ogni frammento visibile:
    Trasformare la posizione world in light space
    Campionare shadow_map[light_space.xy]
    Se shadow_map[light_space.xy] < light_space.z → in ombra
    in_shadow_factor = lerp(1.0, shadow_darkness/100, in_shadow)
    Moltiplicare colore per in_shadow_factor
```

**Shadow Diffusion** = PCF (Percentage Closer Filtering): campionare la shadow map in un kernel NxN intorno al punto e fare media. Kernel size = `shadow_diffusion`.

**Casts Shadows Only**: `layer.material.casts_shadows` ma `!layer.material.accepts_lights` → il layer non appare nel render principale ma contribuisce alla shadow map. Da implementare come flag nel rasterizer.

---

## Phase 3 — Import GLTF (post-MVP)

**Libreria**: `tinygltf` (header-only, MIT license)

```cmake
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG        v2.9.3
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(TINYGLTF_HEADER_ONLY ON CACHE BOOL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(tinygltf)
```

**Struttura dati**:
```cpp
struct GltfMesh {
    std::vector<math::Vector3> vertices;
    std::vector<math::Vector3> normals;
    std::vector<math::Vector2> uvs;
    std::vector<std::uint32_t> indices;  // triangoli
    // Material index
};

struct GltfModel {
    std::vector<GltfMesh> meshes;
    std::vector<AnimationTrack> animations;
    // Camera e light estratte dal file
};
```

Il rasterizer 3D CPU esistente (PerspectiveRasterizer) gestisce già quad UV-mapped. Estenderlo per triangoli arbitrari aggiungendo `draw_triangle()`.

---

## Phase 4 — Image-Based Lighting (IBL)

**Libreria per HDRI**: `stb_image.h` (single-header, già disponibile in molte build)

```cmake
# stb è header-only, basta copiare stb_image.h in third_party/
```

**Pipeline IBL**:
1. Caricare HDRI (formato `.hdr`, float32 per canale).
2. Convertire in cubemap 6 facce (equirectangular → cubemap).
3. Pre-calcolare irradiance map (integrazione emisferica) per l'illuminazione diffusa.
4. Usare l'irradiance map al posto delle luci ambient per illuminare i layer.

Per il MVP: campionamento diretto dell'equirectangular (senza cubemap) — più semplice, visivamente accettabile.

---

## Ordine di implementazione consigliato per domani

```
Mattina (3-4 ore):
  1. Estendere EvaluatedLightState e EvaluatedCameraState con tutti i parametri AE
  2. Estendere EvaluatedLayerState con MaterialOptions e orientation_xyz
  3. Implementare DepthBuffer e integrarlo nel PerspectiveRasterizer
  4. Calcolare world_matrix corretta (TRS con anchor point) in evaluator.cpp

Pomeriggio (3-4 ore):
  5. Implementare apply_lighting_to_layer (ambient + parallel + point + spot)
  6. Implementare compute_falloff
  7. Implementare two-node camera (look_at matrix)
  8. Test: scena con 3 layer 3D, 1 camera two-node, 2 luci (ambient + point)

Sera (1-2 ore):
  9. DOF post-process (blur gaussiano semplificato)
  10. Test di regressione per tutto il 3D path
```

---

## File da creare / modificare

| File | Azione |
|------|--------|
| `include/tachyon/scene/evaluated_state.h` | Estendere EvaluatedLightState, EvaluatedCameraState, EvaluatedLayerState |
| `include/tachyon/renderer2d/depth_buffer.h` | Nuovo |
| `src/renderer2d/depth_buffer.cpp` | Nuovo |
| `src/renderer2d/raster/perspective_rasterizer.cpp` | Integrare DepthBuffer, aggiungere draw_triangle |
| `src/renderer2d/evaluated_composition_renderer.cpp` | apply_lighting_to_layer, DOF, passaggio lights nel render loop |
| `src/scene/evaluator.cpp` | build_layer_world_matrix, two-node camera look_at, compute_child_time per lights |
| `include/tachyon/core/camera/camera_state.h` | Aggiungere look_at, zoom_to_fov |
| `src/CMakeLists.txt` | Aggiungere depth_buffer.cpp |
| `tests/unit/renderer2d/3d_lighting_tests.cpp` | Nuovo — test flat shading, shadow, DOF |

---

## Checklist domani mattina

- [ ] `EvaluatedLightState` ha: cone_angle, cone_feather, falloff, casts_shadows, shadow_darkness, shadow_diffusion, intensity negativa
- [ ] `EvaluatedCameraState` ha: camera_type, point_of_interest, zoom, focal_length, film_size, dof_enabled, focus_distance, aperture, blur_level
- [ ] `EvaluatedLayerState` ha: orientation_xyz, anchor_point_3d, MaterialOptions
- [ ] `DepthBuffer` compila e test_and_write funziona
- [ ] `PerspectiveRasterizer::draw_quad` usa il depth buffer
- [ ] `build_layer_world_matrix` produce TRS corretto (verificare con un layer ruotato 45° su X)
- [ ] `apply_lighting_to_layer` produce risultati visibili su un solid layer bianco con luce point colorata
- [ ] Two-node camera punta correttamente al point of interest (verificare con layer fuori centro)
- [ ] DOF blur si applica quando `dof_enabled = true`
- [ ] Tutti i test esistenti passano ancora (nessuna regressione)
