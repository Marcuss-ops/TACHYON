# Masking e Matte

## Stato attuale

### Cosa esiste
| Componente | File | Stato |
|---|---|---|
| `MaskRasterizer` | `include/tachyon/renderer2d/raster/mask_rasterizer.h` | ✓ alpha-only, feather, winding |
| `MaskPath` | `include/tachyon/renderer2d/path/mask_path.h` | ✓ |
| `Renderer2DMatteResolver` | `include/tachyon/renderer2d/evaluated_composition/renderer2d_matte_resolver.h` | ✓ alpha + luma matte, invert |
| `MatteMode`, `MatteDependency` | `shared_contracts.h` | ✓ |
| `track_matte_layer_id`, `track_matte_type` | `LayerSpec` | ✓ |
| `mask_paths` (vector) | `LayerSpec` | ✓ roto-mask vector |
| `mask_feather` | `LayerSpec` | ✓ keyframeable |

### Gap principali
- `mask_paths` contiene path statici — mancano path animati (vertici keyframeable)
- Nessun tipo "text mask" (testo come sorgente alpha per un altro layer)
- Nessun tipo "shape mask" dichiarativo (usa `ShapeSpec` → genera `MaskPath`)
- `MaskRasterizer` non è cablato al pipeline animated — usa solo il frame corrente

---

## Architettura esistente

Il sistema ha due livelli:

**1. Roto-masks** — path vettoriali su un singolo layer che mascherano il layer stesso.
Il `MaskRasterizer` produce un buffer alpha dalla lista `mask_paths` del layer. Questo alpha viene applicato al layer prima del compositing.

**2. Track matte** — un layer funge da sorgente alpha/luma per un altro layer.
`Renderer2DMatteResolver::resolve()` legge il buffer renderizzato del layer sorgente, lo converte in alpha (per luma matte: luminanza in spazio lineare), e lo applica al layer target.

Entrambi funzionano. I gap sono sulla dichiarazione JSON e sull'animazione delle mask.

---

## Gap 1 — Animated mask paths

Oggi `LayerSpec::mask_paths` è `vector<MaskPath>` statico. Per animare i vertici servono keyframe per vertice.

**Aggiungere a `LayerSpec`:**
```cpp
struct AnimatedMaskVertex {
    AnimatedVector2Spec position;
    AnimatedVector2Spec tangent_in;
    AnimatedVector2Spec tangent_out;
};

struct AnimatedMaskPath {
    std::vector<AnimatedMaskVertex> vertices;
    bool closed{true};
    AnimatedScalarSpec feather;
    bool inverted{false};
    std::string mode{"add"};  // "add", "subtract", "intersect", "difference"
};

// In LayerSpec, sostituire:
std::vector<renderer2d::MaskPath> mask_paths;
// con:
std::vector<AnimatedMaskPath> animated_mask_paths;
```

Nell'evaluator del layer, per ogni frame si campionano i vertici e si costruisce un `MaskPath` concreto da passare al `MaskRasterizer`.

---

## Gap 2 — Shape mask

Oggi per fare una maschera rettangolare bisogna specificare 4 vertici bezier. Con `ShapeSpec` già implementato, è più semplice dichiarare:

```json
{
  "mask_paths": [
    {
      "shape": "rounded_rect",
      "x": 200, "y": 100, "width": 800, "height": 400, "radius": 20,
      "feather": 8.0,
      "inverted": false
    }
  ]
}
```

**Bridge `ShapeSpec → MaskPath`:**
```cpp
renderer2d::MaskPath mask_from_shape_spec(const ShapeSpec& spec) {
    PathGeometry geom = build_geometry(spec);
    // Converti PathGeometry in MaskPath (contours)
    return flatten_to_mask_path(geom);
}
```

Questo riusa `ShapeFactory` già esistente senza duplicare logica.

---

## Gap 3 — Text mask

Un layer di testo come maschera per un altro layer (es: titolo riempito da video).

Pipeline:
1. Rasterizza il layer testo come al solito → superficie RGBA
2. Usa il canale alpha del testo come matte per il layer video sottostante

Questo è già supportato dall'architettura `Renderer2DMatteResolver` con `MatteMode::Alpha`.
Manca solo il parsing JSON:

```json
{
  "id": "video_fill",
  "type": "video",
  "src": "footage.mp4",
  "track_matte_layer_id": "title_text",
  "track_matte_type": "alpha"
}
```

**Verifica:** `track_matte_layer_id` e `track_matte_type` esistono già in `LayerSpec`. Se il compositor chiama già `Renderer2DMatteResolver::resolve()`, il text mask dovrebbe funzionare senza codice aggiuntivo — solo testarlo.

---

## Tipi matte supportati (già in `MatteMode`)

```cpp
enum class MatteMode {
    None,
    Alpha,        // usa canale alpha del layer sorgente
    AlphaInverted,
    Luma,         // usa luminanza del layer sorgente (lineare)
    LumaInverted
};
```

---

## JSON completo — esempi pratici

**Testo che appare dentro una barra (clip con rect mask animata):**
```json
{
  "id": "text_layer",
  "type": "text",
  "text_content": "TACHYON ENGINE",
  "animated_mask_paths": [
    {
      "shape": "rectangle",
      "x": {"keyframes": [
        {"time": 0.0, "value": -1920},
        {"time": 0.8, "value": 0, "easing": "ease_out"}
      ]},
      "y": 0, "width": 1920, "height": 200
    }
  ]
}
```

**Immagine rivelata da linea (wipe con luma matte):**
```json
{"id": "wipe_matte", "type": "solid",
 "fill_color": [255,255,255,255],
 "animated_mask_paths": [{
   "shape": "rectangle",
   "width": {"keyframes": [
     {"time": 0.0, "value": 0},
     {"time": 1.2, "value": 1920}
   ]},
   "height": 1080
 }]},
{"id": "reveal_image", "type": "image", "src": "photo.jpg",
 "track_matte_layer_id": "wipe_matte", "track_matte_type": "luma"}
```

**Titolo riempito da video:**
```json
{"id": "title", "type": "text", "text_content": "BREAKING NEWS", "font_size": 120},
{"id": "video_inside", "type": "video", "src": "news_bg.mp4",
 "track_matte_layer_id": "title", "track_matte_type": "alpha"}
```

---

## Feather

`MaskRasterizer::Config::feather` è già disponibile. Nel JSON:
```json
{"mask_paths": [{"shape": "circle", "cx": 960, "cy": 540, "radius": 400, "feather": 60}]}
```

Il feather è un blur gaussiano applicato al buffer alpha dopo la rasterizzazione:
```cpp
if (cfg.feather > 0.0f) {
    blur_alpha_buffer(out_alpha, width, height, cfg.feather);
}
```

`blur_alpha_mask()` esiste già in `effect_common.h`.

---

## Ordine di implementazione

```
1. Verificare che track matte (alpha/luma) funzioni end-to-end con text/shape sorgente
2. Shape mask: bridge ShapeSpec → MaskPath nel parser JSON
3. Animated mask paths: AnimatedMaskPath struct + evaluator per vertici
4. Feather animato: AnimatedScalarSpec per feather in AnimatedMaskPath
5. Mask modes combinati (add/subtract/intersect) → boolean ops via Clipper2 già disponibile
```
