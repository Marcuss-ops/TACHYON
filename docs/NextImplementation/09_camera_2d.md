# Camera 2D

## Stato attuale

### Cosa esiste
| Componente | File | Stato |
|---|---|---|
| `CameraState` (3D) | `include/tachyon/core/camera/camera_state.h` | ✓ completo — view/projection/MVP matrix |
| `camera_type`, `camera_zoom`, `camera_poi` | `LayerSpec` | ✓ spec esistente |
| `camera_shake_*` (seed, amplitude, frequency, roughness) | `LayerSpec` | ✓ keyframeable |
| `camera_cuts.h` | `include/tachyon/timeline/camera_cuts.h` | ✓ |
| `EvaluatedCameraState` | `include/tachyon/core/scene/state/evaluated_camera_state.h` | ✓ |

### Cosa manca
La `CameraState` esistente è 3D (prospettiva, focal length). Per composizioni 2D serve un sistema separato: la camera non è un layer ma un **viewport transform** applicato all'intera composizione prima del compositing.

Pan, zoom, rotate e shake in 2D non richiedono prospettiva — sono una `Transform2D` applicata al framebuffer finale o alla posizione di ogni layer.

---

## Architettura da implementare

### Modello concettuale
```
[Composizione 2D] — layer stack valutati normalmente
        ↓
[Camera2D Transform] — pan, zoom, rotate applicato come viewport
        ↓
[Framebuffer output]
```

La camera non modifica i layer. Trasforma il **punto di vista** da cui la composizione viene letta nel framebuffer di output.

### Strutture da aggiungere

**`include/tachyon/core/spec/schema/objects/composition_spec.h`** — aggiungere:
```cpp
struct Camera2DSpec {
    // Pivot point (composition center by default)
    float pivot_x{0.0f};
    float pivot_y{0.0f};

    // Animated properties (keyframeable)
    AnimatedVector2Spec position;    // pan offset in pixels
    AnimatedScalarSpec  zoom;        // 1.0 = no zoom, 2.0 = 2x
    AnimatedScalarSpec  rotation;    // degrees
    AnimatedScalarSpec  shake_amplitude_pos;
    AnimatedScalarSpec  shake_amplitude_rot;
    AnimatedScalarSpec  shake_frequency;
    std::uint64_t       shake_seed{0};

    bool enabled{false};
};
```

**`include/tachyon/core/scene/state/evaluated_state.h`** — aggiungere a `EvaluatedCompositionState`:
```cpp
struct EvaluatedCamera2D {
    float pan_x{0.0f};
    float pan_y{0.0f};
    float zoom{1.0f};
    float rotation_deg{0.0f};
    math::Transform2 viewport_transform;
};
std::optional<EvaluatedCamera2D> camera_2d;
```

---

## Step 1 — Evaluator camera 2D

**`src/core/scene/evaluator/camera2d_evaluator.cpp`** — nuovo file:
```cpp
EvaluatedCamera2D evaluate_camera_2d(
    const Camera2DSpec& spec,
    double time_seconds,
    uint32_t frame_number)
{
    EvaluatedCamera2D cam;
    auto pos = sample_vector2(spec.position, time_seconds);
    cam.pan_x = pos.x;
    cam.pan_y = pos.y;
    cam.zoom  = static_cast<float>(sample_scalar(spec.zoom, 1.0, time_seconds));
    cam.rotation_deg = static_cast<float>(sample_scalar(spec.rotation, 0.0, time_seconds));

    // Camera shake: noise-based offset seeded per frame
    float shake_amp = static_cast<float>(sample_scalar(spec.shake_amplitude_pos, 0.0, time_seconds));
    if (shake_amp > 0.0f) {
        float freq = static_cast<float>(sample_scalar(spec.shake_frequency, 8.0, time_seconds));
        float t = static_cast<float>(time_seconds) * freq;
        cam.pan_x += shake_amp * noise1d(spec.shake_seed,     t);
        cam.pan_y += shake_amp * noise1d(spec.shake_seed + 1, t);
    }

    // Build viewport transform: scale → rotate → translate
    cam.viewport_transform = math::Transform2::identity();
    cam.viewport_transform = cam.viewport_transform.scaled(cam.zoom, cam.zoom);
    cam.viewport_transform = cam.viewport_transform.rotated(cam.rotation_deg * kDegToRad);
    cam.viewport_transform = cam.viewport_transform.translated(cam.pan_x, cam.pan_y);
    return cam;
}
```

---

## Step 2 — Applicazione nel compositor

**`src/renderer2d/evaluated_composition/rendering/composition_renderer.cpp`** — dopo aver composited tutti i layer nel framebuffer intermedio, prima di scrivere l'output:

```cpp
if (composition_state.camera_2d.has_value()) {
    const auto& cam = *composition_state.camera_2d;
    // Applica il viewport transform con bilinear sampling
    apply_camera2d_transform(output_framebuffer, cam.viewport_transform, comp_width, comp_height);
}
```

**`apply_camera2d_transform`** — inverti la transform per ogni pixel di output, campiona con bilinear:
```cpp
void apply_camera2d_transform(SurfaceRGBA& dst, const Transform2& cam_to_world, int w, int h) {
    auto world_to_cam = cam_to_world.inverted();
    SurfaceRGBA src = dst; // copia
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            auto world_pt = world_to_cam.transform_point({(float)x, (float)y});
            dst.set_pixel(x, y, src.sample_bilinear(world_pt.x, world_pt.y));
        }
    }
}
```

---

## Step 3 — Parallax layers

Il parallax si ottiene **prima** di applicare la camera globale. Ogni layer ha un campo `parallax_factor` (0.0 = fisso, 1.0 = si muove con la camera, 0.5 = si muove a metà):

```cpp
// In LayerSpec aggiungere:
float parallax_factor{1.0f};
```

Nell'evaluator, la posizione di ogni layer viene traslata di:
```cpp
layer_state.local_transform.position.x += camera_pan_x * layer.parallax_factor;
layer_state.local_transform.position.y += camera_pan_y * layer.parallax_factor;
```

Il background (factor 0) rimane fisso. Il testo in primo piano (factor 1.2) si muove più veloce. Questo simula la profondità senza prospettiva reale.

---

## JSON finale

```json
{
  "id": "main_comp",
  "width": 1920, "height": 1080,
  "camera_2d": {
    "enabled": true,
    "zoom": {"keyframes": [
      {"time": 0.0, "value": 1.0},
      {"time": 5.0, "value": 1.15}
    ]},
    "position": {"keyframes": [
      {"time": 0.0, "value": [0, 0]},
      {"time": 5.0, "value": [-80, 20]}
    ]},
    "shake_amplitude_pos": 0.0,
    "shake_frequency": 8.0,
    "shake_seed": 42
  },
  "layers": [
    {"id": "bg",   "type": "image", "parallax_factor": 0.0},
    {"id": "mid",  "type": "image", "parallax_factor": 0.5},
    {"id": "text", "type": "text",  "parallax_factor": 1.0}
  ]
}
```

---

## Ordine di implementazione

```
1. Camera2DSpec in CompositionSpec + parser JSON
2. EvaluatedCamera2D in EvaluatedCompositionState
3. camera2d_evaluator.cpp — evaluate_camera_2d()
4. apply_camera2d_transform() nel compositor
5. parallax_factor in LayerSpec + applicazione nell'evaluator layer
6. Follow object: calcola pan automatico in base alla posizione di un layer target
```

Item 4 è il core — tutto il resto si aggiunge sopra senza cambiare l'architettura.
