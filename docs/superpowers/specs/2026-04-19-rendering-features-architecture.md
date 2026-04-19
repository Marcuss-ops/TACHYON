# Tachyon — Rendering Features Architecture

**Data**: 2026-04-19  
**Scope**: architettura tecnica di riferimento per le 9 feature di rendering prioritizzate  
**Stato codebase al momento della stesura**: branch `main`, commit `b57c5fc`

---

## Panoramica priorità

| # | Feature | Stato attuale | Complessità | Note |
|---|---------|--------------|-------------|------|
| 1 | Motion blur | ~85% | Bassa | Blend mode mancante è il vero gap |
| 2 | Adjustment layers | Stub | Media | Refactor loop compositor |
| 3 | Color management primaries | ~65% | Bassa | Matrici statiche, no librerie |
| 4 | ROI / inter-frame cache | Parziale | Media | Due problemi separati |
| 5 | Text shaping | Bassa | Alta | Dipendenza HarfBuzz+FreeType |
| 6 | Audio mux | Contratto only | Media | Two-pass ffmpeg consigliato |
| 7 | Low-end policy | Stringa only | Bassa | Logica semplice, alta copertura |
| 8 | Timeline (time remap) | Compatibilità | Bassa | Usa evaluator esistente |
| 9 | 3D CPU | Parziale | Alta | Depth buffer primo passo |

---

## Roadmap sprint

### Sprint 1

- Blend mode nel compositing 2D e nel motion blur
- Default motion blur samples quando il job richiede blur ma passa `0`
- Sample distribution iniziale: `box`, con estensione a `triangle` e `gaussian`
- Test di regressione sul compositing additivo/multiply/screen

### Sprint 2

- Color management primaries e range
- Quality policy engine con `draft`, `preview`, `high`, `cinematic`
- Propagazione della policy nel runtime e nel renderer

### Sprint 3

- Adjustment layers con compositing parziale
- Time remap per i precomp layer
- Precomp cache inter-frame nel `RenderContext`

### Sprint 4

- ROI / tiling vero
- Text shaping con HarfBuzz + FreeType
- Audio mux two-pass

### Sprint 5

- 3D CPU: depth buffer e luci base
- Shadow map e geometria estrusa solo dopo la base stabile

---

## Ondata 1: Fondamenta e Correttezza

Questa ondata raccoglie i quick wins che correggono bug strutturali o sbloccano il potenziale del codice esistente.

### 1. Motion Blur & Blend Modes

**Gap stimato**: 20%

**Quanto manca**: il loop di accumulo esiste già, ma mancano i blend mode corretti nel compositing e il motion blur kernel selezionabile. Senza blend mode, il motion blur brucia i bianchi o fallisce sui layer semitrasparenti.

**Implementazione corretta**:

1. `SurfaceRGBA`: aggiungere un metodo `composite_at(source, x, y, opacity, BlendMode)`.
2. `Blend Functions`: implementare le formule standard, per esempio `Additive = clamp(dst + src * opacity)`.
3. `Kernel`: in `render_composition_recursive`, calcolare il weight del campione in base a `plan.motion_blur_curve` invece di usare solo `1.0 / num_samples`.

### 2. Low-End Policy Engine

**Gap stimato**: 90%

**Quanto manca**: esiste il parametro `quality_tier` nel piano, ma il renderer lo ignora.

**Implementazione corretta**:

1. Creare `QualityPolicy.h` con una `struct` che mappa il tier a valori concreti come `res_scale`, `mb_sample_cap`, `tile_size`.
2. `RenderSession`: all'inizio del job, istanziare la policy.
3. `Resolution Scale`: se la policy dice `0.5`, allocare `frame_surface` a metà risoluzione e fare upscale finale bilinear solo nel sink.

### 3. Time Remapping

**Gap stimato**: 80%

**Quanto manca**: il renderer assume un tempo lineare. Se un precomp è rallentato o invertito, Tachyon non lo sa.

**Implementazione corretta**:

1. In `Evaluator.cpp`, quando valuti un layer, calcolare `child_time = interpolate(layer.time_remap_curve, parent_time)`.
2. In `evaluated_composition_renderer.cpp`, passare questo `child_time` alla chiamata ricorsiva del precomp.

---

## Ondata 2: Efficienza e Pipeline

Questa ondata rende Tachyon capace di gestire scene professionali senza crashare per memoria o latenza inutile.

### 4. ROI & Tiling

**Gap stimato**: 85%

**Quanto manca**: esiste il concetto di `tile_rect`, ma viene passato sempre l'intero frame.

**Implementazione corretta**:

1. In `render_evaluated_composition_2d`, dividere il frame in una griglia, per esempio `256x256`.
2. Culling: prima di renderizzare un layer, usare `intersect_rects(layer_bounds, tile_rect)`. Se vuoto, `continue`.
3. Questo riduce drasticamente l'uso di memoria su scene 4K con molti layer piccoli.

### 5. Inter-frame Precomp Cache

**Gap stimato**: 70%

**Quanto manca**: la cache attuale muore alla fine di ogni frame.

**Implementazione corretta**:

1. Spostare `PrecompCache` in `RenderSession`, così sopravvive tra i frame.
2. Cache key: la chiave deve includere `composition_hash + child_frame_number`.
3. Se un precomp è statico, per esempio un logo, viene renderizzato una sola volta per l'intero export.

### 6. Color Management Primaries

**Gap stimato**: 80%

**Quanto manca**: gestiamo la curva, cioè la gamma, ma non i primari, cioè il "colore" del rosso, verde e blu.

**Implementazione corretta**:

1. Aggiungere una matrice `3x3` di conversione in `color_transfer.h`, per esempio `Rec709 -> P3`.
2. Applicare la matrice pixel-per-pixel nel sink prima della codifica.

---

## 1. Motion blur

### Stato preciso

Il pipeline sub-frame è **già funzionante**:
- `build_render_plan` popola sempre `scene_spec` (`render_plan.cpp:84`)
- `evaluate_scene_composition_state(scene, id, double time_seconds)` interpola keyframe a tempo arbitrario
- `AccumulationBuffer` accumula in linear premultiplied, risolve correttamente

### Gap reali

**A) `composite_surface_at` ignora il blend mode** (`evaluated_composition_renderer.cpp:119`):
```cpp
(void)mode;  // tutti i layer usano src-over
```
Motion blur accumula campioni dove ogni campione è già blendato in modo errato per layer Additive/Multiply/Screen.

**B) Curva di esposizione**: `motion_blur_curve` è nel `RenderPlan` ma solo distribuzione uniforme (box) è implementata.

**C) Guard mancante**: `motion_blur_samples == 0` con `motion_blur_enabled == true` deve defaultare a 8.

### Implementazione

**Passo 1 — Blend mode in `composite_surface_at`:**
```cpp
void composite_surface_at(SurfaceRGBA& dest, const SurfaceRGBA& src,
                           int x, int y, const RectI& clip,
                           renderer2d::BlendMode mode) {
    // Per ogni pixel (cx, cy) nell'intersezione:
    const Color s = src.get_pixel(cx - x, cy - y);
    const Color d = dest.get_pixel(cx, cy);
    Color result;
    switch (mode) {
        case BlendMode::Normal:
            result = composite_src_over(s, d, working_curve);
            break;
        case BlendMode::Additive:
            // premult src + premult dst, clamp
            result = blend_additive(s, d, working_curve);
            break;
        case BlendMode::Multiply:
            result = blend_multiply(s, d, working_curve);
            break;
        case BlendMode::Screen:
            result = blend_screen(s, d, working_curve);
            break;
    }
    dest.set_pixel(cx, cy, result);
}
```

Le formule per tutti i blend mode sono in `color_transfer.h` — aggiungere `blend_additive`, `blend_multiply`, `blend_screen` come inline accanto a `composite_src_over`.

**Passo 2 — Kernel di campionamento** in `render_composition_recursive`:
```cpp
auto sample_weight = [&](int sample, int total) -> float {
    if (total == 1) return 1.0f;
    const float t = static_cast<float>(sample) / (total - 1) * 2.0f - 1.0f; // [-1, 1]
    if (plan.motion_blur_curve == "gaussian") {
        const float sigma = 0.4f;
        return std::exp(-t * t / (2.0f * sigma * sigma));
        // normalizzare dividendo per la somma totale dei pesi
    }
    if (plan.motion_blur_curve == "triangle") {
        return 1.0f - std::abs(t);
    }
    return 1.0f; // box
};
// Calcolare i pesi, normalizzarli, usarli in accumulation.add()
```

**Costo stimato**: 1-2 giorni.

---

## 2. Adjustment layers

### Semantica AE

Un adjustment layer applica i suoi effetti all'immagine composita di tutti i layer sotto di lui nello stack (nell'area coperta dal layer stesso). Opacity del layer controlla il blend wet/dry.

### Architettura

Il loop attuale in `render_composition_recursive` non ha accesso alla superficie parziale quando processa ogni layer. Va modificato così:

```
Loop bottom-to-top su sample_layers:
    if layer.type == Camera: skip
    if !layer.visible: skip
    
    if layer.is_adjustment_layer:
        apply_adjustment(layer, sample_surface, width, height, plan, task)
        // sample_surface è modificata in-place nella bbox del layer
    else:
        render_layer_recursive(layer, ..., sample_surface)
```

**`apply_adjustment`:**
```cpp
void apply_adjustment(
    const scene::EvaluatedLayerState& layer,
    SurfaceRGBA& composite,       // la superficie corrente (tutto sotto)
    std::int64_t width, std::int64_t height,
    const RenderPlan& plan,
    const FrameRenderTask& task)
{
    const RectI bbox = layer_bounds(layer, width, height);
    // Estrarre la regione, applicare effetti, reinserire con opacity blend
    SurfaceRGBA region = composite.extract(bbox);
    apply_effects_to_surface(region, layer.effects, plan, task);
    composite.blend_region(region, bbox, static_cast<float>(layer.opacity));
}
```

**Mask su adjustment layer**: la maschera del layer definisce il falloff dell'effetto. Si applica come alpha mask sulla differenza `(effected - original)` prima del blend.

**Effetti minimi necessari** (già parzialmente in `effect_host.cpp`):
- Brightness/Contrast
- Hue/Saturation  
- Color Balance (lift/gamma/gain)
- Gaussian Blur
- Tint

**Costo stimato**: 2-3 giorni.

---

## 3. Color management — primaries e range

### Stato attuale

`png_sequence_sink.cpp:91` chiama già `convert_frame(frame, source_transfer, output_transfer)` — la conversione di transfer curve avviene. Manca la conversione di primaries e il range clamp.

### Primaries

sRGB e Rec.709 hanno le stesse primaries — differiscono solo in transfer curve. La conversione di primaries è necessaria solo per DCI-P3, Rec.2020, ACES AP0/AP1.

```cpp
// In color_transfer.h
enum class ColorPrimaries {
    Rec709,   // = sRGB primaries
    DciP3D65, // Display P3
    Rec2020,
    AcesAP1,
    AcesAP0
};

// Matrici 3x3 RGB→XYZ (D65 white point, valori standard ITU/SMPTE)
// Rec709→XYZ: nota come M_sRGB
// Conversione: M_dst_inv * M_src applicata per pixel

struct Mat3x3 { float m[9]; };

inline Mat3x3 primaries_conversion_matrix(ColorPrimaries src, ColorPrimaries dst);

inline Color apply_primaries_matrix(Color color, const Mat3x3& m) {
    // Operare in float lineare
    const float r = ...; // linearizzare
    const float rr = m[0]*r + m[1]*g + m[2]*b;
    // ...
}
```

Le 5 matrici sono costanti note — valori da IEC 61966-2-1, ITU-R BT.709, SMPTE ST 2065-1.

### Range

```cpp
inline Color apply_limited_range(Color color) {
    // Y: [16, 235], Cb/Cr: [16, 240]
    // Per RGB (non YCbCr): scala [0,255] → [16, 235]
    auto scale = [](uint8_t v) -> uint8_t {
        return static_cast<uint8_t>(16 + (v * 219) / 255);
    };
    return Color{scale(color.r), scale(color.g), scale(color.b), color.a};
}
```

### Pipeline output completa

```
1. resolve() → frame in working_space (linear o con transfer)
2. apply_primaries_matrix(frame, working_primaries, output_primaries)
   — solo se diversi
3. apply_transfer_curve(frame, working_transfer → output_transfer)
   — già in convert_frame()
4. apply_range(frame, output_range)
   — se "limited"
```

**Aggiungere `parse_color_primaries(string)` → `ColorPrimaries`** e aggiornare entrambi i sink (PNG e ffmpeg).

**Costo stimato**: 1.5 giorni.

---

## 4. ROI e inter-frame cache

### 4A. Tiling reale

**Problema**: il tile passato a `render_layer_recursive` è sempre `{0, 0, width, height}`. Il culling per-layer esiste ma il tile è sempre il frame intero, quindi nessun risparmio di memoria.

**Architettura tiled rendering:**

```cpp
// In render_composition_recursive, sostituire il loop layer singolo con:

const int tile_size = quality_policy.tile_size; // 0 = no tiling
if (tile_size == 0) {
    // path attuale, tile = full frame
} else {
    for (int ty = 0; ty < height; ty += tile_size) {
        for (int tx = 0; tx < width; tx += tile_size) {
            const RectI tile{tx, ty,
                             std::min(tile_size, (int)width - tx),
                             std::min(tile_size, (int)height - ty)};
            SurfaceRGBA tile_surface(tile.width, tile.height);
            tile_surface.clear(Color::transparent());
            
            for (const auto& layer : sample_layers) {
                // layer_bounds() + intersect_rects() → skip se vuoto
                render_layer_recursive(layer, ..., tile, ..., tile_surface);
            }
            
            // Blit tile_surface → frame.surface at (tx, ty)
            frame.surface->blit(tile_surface, tx, ty);
        }
    }
}
```

Vantaggio memoria: picco = `tile_size² * 4 bytes` invece di `width * height * 4 bytes`.

**Path 3D**: aggiungere clip post-projection nel perspective rasterizer. Se tutti i vertici proiettati sono fuori tile, skip prima del rasterize.

### 4B. Inter-frame precomp cache

**Problema**: `RenderContext::precomp_cache` vive in `render_evaluated_composition_2d()` — distrutta ad ogni frame.

**Struttura da aggiungere:**

```cpp
// renderer2d/precomp_cache.h
class PrecompCache {
public:
    struct Entry {
        RasterizedFrame2D frame;
        std::size_t size_bytes;
        std::int64_t last_used_frame;
    };

    explicit PrecompCache(std::size_t max_bytes);
    void insert(const std::string& key, RasterizedFrame2D frame);
    const RasterizedFrame2D* lookup(const std::string& key);
    void evict_until_under_budget();

private:
    std::unordered_map<std::string, Entry> m_entries;
    std::size_t m_max_bytes;
    std::size_t m_current_bytes{0};
};
```

**Cache key per precomp:**
```
key = precomp_layer_id + "|" + child_frame_number + "|" + nested_comp_hash
```

`nested_comp_hash` = hash delle proprietà del nested composition (layout identico alla frame cache key). Se il nested comp è statico, stesso `child_frame_number` → cache hit tra frame diversi del parent.

**Propagazione:**
```
RenderSession::render()
    → PrecompCache cache(policy.precomp_cache_budget)
    → execute_frame_task(scene, plan, task, frame_cache, &precomp_cache)
    → render_evaluated_composition_2d(state, plan, task, &precomp_cache)
    → RenderContext::shared_precomp_cache = precomp_cache
```

**Costo stimato**: 2 giorni.

---

## 5. Text shaping

### Stack

- **HarfBuzz**: shaping (glyph IDs, advances, kerning, ligature)
- **FreeType**: rasterizzazione glyph → bitmap
- **Glyph atlas**: cache bitmap per (font, size, glyph_id)

### Architettura

```cpp
// text/shaper.h
struct ShapedGlyph {
    std::uint32_t glyph_id;
    float advance_x;
    float offset_x, offset_y;
    int cluster_index;       // posizione nel testo UTF-8
    math::Vector2 position;  // posizione assoluta nel layer dopo layout
};

struct ShapedLine {
    std::vector<ShapedGlyph> glyphs;
    float width;
    float ascender;
    float descender;
};

std::vector<ShapedLine> shape_text(
    const std::string& utf8_text,
    const FontFace& face,
    float size_px,
    float max_width,             // 0 = no wrap
    TextAlignment alignment);
```

**Pipeline:**
```
1. Caricare font con FreeType → FT_Face
2. Creare hb_font da FT_Face
3. hb_buffer_add_utf8() + hb_shape() → glyph IDs + positions
4. Line breaking su advance_x accumulato
5. Alignment: aggiustare x offset per left/center/right
6. Per ogni glyph: FT_Load_Glyph + FT_Render_Glyph → bitmap
7. Cache bitmap in GlyphAtlas (font+size+glyph_id → FT_Bitmap*)
8. Blit bitmap sulla layer surface alla posizione shapeata
```

### Glyph animation (AE Range Selector style)

```cpp
struct TextAnimator {
    float range_start{0.0f};  // 0..1 della stringa
    float range_end{1.0f};
    float range_smoothness{0.0f};
    // Proprietà animate per glyph nel range:
    std::optional<math::Vector2> position_offset;
    std::optional<float> opacity;
    std::optional<float> rotation;
    std::optional<float> scale;
};

// Post-shaping, per ogni glyph:
for (int i = 0; i < glyphs.size(); i++) {
    const float t = (float)i / glyphs.size(); // selettore 0..1
    for (const auto& anim : layer.text_animators) {
        if (t >= anim.range_start && t <= anim.range_end) {
            const float factor = compute_range_factor(anim, t);
            glyphs[i].position += anim.position_offset * factor;
            // ... altri attributi
        }
    }
}
```

### Dipendenze CMake

```cmake
find_package(Freetype REQUIRED)
find_package(HarfBuzz REQUIRED)
target_link_libraries(tachyon_renderer Freetype::Freetype harfbuzz::harfbuzz)
```

Su Windows: `vcpkg install freetype harfbuzz`. Su Linux: `apt install libfreetype6-dev libharfbuzz-dev`.

**Costo stimato**: 3-4 giorni integrazione base + 1-2 glyph animation.

---

## 6. Audio mux

### Approccio consigliato: two-pass ffmpeg

**Perché two-pass**: il muxing streaming interleaved richiede timing preciso dei pacchetti audio/video nell'ordine corretto, che è complesso da gestire. Two-pass è robusto e semplice.

```
Pass 1: render video → file temporaneo (già funzionante)
Pass 2: ffmpeg -i video_tmp.mp4 -i audio_mixed.wav 
        -c:v copy -c:a aac output.mp4
```

### AudioMixer

```cpp
// audio/audio_mixer.h
class AudioMixer {
public:
    struct Track {
        std::string file_path;
        double in_point_seconds;
        double out_point_seconds;
        float volume{1.0f};
        double time_remap_speed{1.0f};
    };

    void add_track(Track track);
    
    // Produce PCM float interleaved per il range di frames dato
    std::vector<float> mix(double start_seconds,
                           double end_seconds,
                           int sample_rate,
                           int channels);
    
    // Scrive WAV nel path dato
    bool write_wav(const std::filesystem::path& path,
                   double duration_seconds,
                   int sample_rate,
                   int channels);
};
```

Il mixer decodifica ogni file audio con ffmpeg (come decoder, non encoder), applica volume e time remap, somma le tracce, clamp in [-1, 1].

### Integrazione in RenderSession

```cpp
// Dopo render video:
AudioMixer mixer;
for (const auto& audio_layer : audio_layers) {
    mixer.add_track({audio_layer.file_path, audio_layer.in_point, ...});
}
const auto temp_audio = std::filesystem::temp_directory_path() / "tachyon_audio.wav";
mixer.write_wav(temp_audio, duration, sample_rate, channels);

// ffmpeg mux pass
std::string cmd = "ffmpeg -i " + video_temp.string()
    + " -i " + temp_audio.string()
    + " -c:v copy -c:a " + plan.output.profile.audio.codec
    + " " + final_output.string();
// ...
```

**Costo stimato**: 2-3 giorni.

---

## 7. Low-end policy engine

### Struttura

```cpp
// runtime/quality_policy.h
struct QualityPolicy {
    float resolution_scale{1.0f};
    int motion_blur_sample_cap{64};
    bool effects_enabled{true};
    bool precomp_cache_enabled{true};
    std::size_t precomp_cache_budget{1ULL * 1024 * 1024 * 1024};
    int tile_size{0};            // 0 = full frame
    std::size_t max_workers{0};  // 0 = tutti i core
};

QualityPolicy make_quality_policy(const std::string& tier);
```

### Tier predefiniti

| Tier | res_scale | mb_samples_cap | effects | cache_budget | tile_size |
|------|-----------|---------------|---------|-------------|-----------|
| `draft` | 0.5 | 2 | false | 64 MB | 256 |
| `preview` | 0.75 | 4 | true | 256 MB | 512 |
| `high` | 1.0 | 16 | true | 1 GB | 0 |
| `cinematic` | 1.0 | 64 | true | 4 GB | 0 |

### Punti di applicazione

```
RenderSession::render():
    QualityPolicy policy = make_quality_policy(plan.quality_tier);
    
    // Resolution scale: render a dimensioni ridotte
    const int effective_w = plan.composition.width * policy.resolution_scale;
    const int effective_h = plan.composition.height * policy.resolution_scale;
    // dopo render, upscale bilinear → dimensioni originali per output
    
    // Worker cap:
    worker_count = min(worker_count, policy.max_workers > 0 ? policy.max_workers : worker_count)

render_composition_recursive():
    // Motion blur cap:
    const int num_samples = plan.motion_blur_enabled
        ? std::clamp((int)plan.motion_blur_samples, 1, policy.motion_blur_sample_cap)
        : 1;
    
    // Effects skip:
    if (!policy.effects_enabled) { skip apply_effects_to_surface() }
    
    // Tile size:
    use policy.tile_size per il tiling loop
```

**Costo stimato**: 1.5 giorni.

---

## 8. Timeline — time remapping

### Problema

Quando un precomp layer viene renderizzato in `render_layer_recursive`, usa `task.frame_number` del parent. Non c'è time remapping: no slow-motion, no freeze, no reverse.

### Architettura

**Dato nel layer spec** (aggiungere a `LayerSpec` / `EvaluatedLayerState`):
```cpp
struct TimeRemap {
    bool enabled{false};
    std::vector<Keyframe<double>> curve; // parent_time_sec → child_time_sec
};
```

**Nel valutatore:**
```cpp
double compute_child_time(const EvaluatedLayerState& layer,
                           double parent_time_seconds) {
    if (!layer.time_remap.enabled) {
        return parent_time_seconds - layer.in_point_seconds;
    }
    return interpolate_keyframes(layer.time_remap.curve, parent_time_seconds);
}
```

**Nel renderer**: quando si renderizza un precomp nested, invece di usare `task.frame_number`:
```cpp
// render_layer_recursive, nel branch Precomp:
const double child_time = compute_child_time(layer, sample_time);
const auto child_state = scene::evaluate_composition_state(
    *nested_comp_spec, child_time);
// render child_state invece di layer.nested_composition
```

**Synergy con inter-frame precomp cache**: se la curva di time remap è piatta (freeze frame), `child_time` è costante → stessa cache key per più frame parent → massima efficacia della cache.

**Costo stimato**: 1.5 giorni.

---

## 9. 3D CPU

### Scope realistico (AE-style)

Non un renderer 3D general-purpose. Layer = card planari in 3D spazio. Camera proiettiva. Luci flat-shaded (per layer, non per pixel).

### A) Depth buffer (prerequisito)

```cpp
// renderer2d/depth_buffer.h
struct DepthBuffer {
    std::vector<float> inv_z; // 1/z per prospective-correct
    std::uint32_t width, height;

    DepthBuffer(std::uint32_t w, std::uint32_t h)
        : inv_z(w * h, 0.0f), width(w), height(h) {}

    bool test_and_write(int x, int y, float inv_z_val) {
        const std::size_t idx = y * width + x;
        if (inv_z_val <= inv_z[idx]) return false; // occultato
        inv_z[idx] = inv_z_val;
        return true;
    }
    
    void clear() { std::fill(inv_z.begin(), inv_z.end(), 0.0f); }
};
```

Passare `DepthBuffer*` al perspective rasterizer. Per ogni frammento: `depth_buffer->test_and_write(x, y, fragment_inv_z)`. Rimuovere il depth sorting by-layer (che fallisce con layer intersecanti).

### B) Luci

```cpp
struct EvaluatedLightState {
    std::string type; // "ambient" | "parallel" | "point" | "spot"
    math::Vector3 position;
    math::Vector3 direction; // normalizzato
    Color color;
    float intensity{1.0f};
    float cone_angle{45.0f};   // spot
    float cone_feather{50.0f}; // spot
    float attenuation_near{0.0f};
    float attenuation_far{500.0f};
};
```

**Calcolo illuminazione per layer card** (flat shading):
```cpp
Color apply_lighting(Color base_color,
                     const math::Vector3& layer_normal,
                     const std::vector<EvaluatedLightState>& lights,
                     const math::Vector3& layer_world_pos) {
    float r = 0, g = 0, b = 0;
    for (const auto& light : lights) {
        if (light.type == "ambient") {
            r += base_r * light.color.r * light.intensity;
            // ...
        }
        if (light.type == "parallel") {
            const float ndotl = std::max(0.0f, dot(layer_normal, -light.direction));
            r += base_r * light.color.r * light.intensity * ndotl;
            // ...
        }
        if (light.type == "point") {
            const auto to_light = (light.position - layer_world_pos).normalized();
            const float ndotl = std::max(0.0f, dot(layer_normal, to_light));
            const float dist = (light.position - layer_world_pos).length();
            const float atten = 1.0f - std::clamp(
                (dist - light.attenuation_near) /
                (light.attenuation_far - light.attenuation_near), 0.0f, 1.0f);
            r += base_r * light.color.r * light.intensity * ndotl * atten;
        }
        // spot: ndotl + cone angle test
    }
    return Color{clamp_u8(r), clamp_u8(g), clamp_u8(b), base_color.a};
}
```

`layer_normal` = Z-forward del layer dopo la sua rotazione 3D.

### C) Shape extrusion

```
ShapeLayer 3D → extrude_path(path, depth) → TriMesh:
    front face: path triangolato (earcut o fan su poly convessa)
    back face:  front traslato di -depth sul Z
    sides:      quad per ogni segmento del path border
                (2 triangoli per segment × N segments)
```

Il mesh risultante usa il pipeline perspective rasterizer già esistente.

### D) Ombre (solo `cinematic`)

Shadow map ortografica per luci `parallel`:
```
1. Render della scena dalla prospettiva della luce (proiezione ortografica)
   → depth texture in shadow_map
2. Per ogni fragment nella render principale:
   transform fragment_pos in light space
   if shadow_map.depth(light_space_pos) < fragment_depth → in ombra
   apply shadow_intensity (0 = piena ombra, 1 = no ombra)
```

### Ordine di implementazione consigliato

1. Depth buffer (rimuove depth-sort, prerequisito)
2. Luci ambient + parallel (95% casi d'uso AE)
3. Point lights
4. Shape extrusion
5. Spot lights
6. Ombre (`cinematic` only)

**Costo stimato**: 1 settimana per 1+2+3, 3-4 giorni aggiuntivi per shape extrusion, 3-4 per ombre.

---

## Ordine di implementazione suggerito

Considerando dipendenze e ritorno sul valore:

```
Sprint 1 (bassa complessità, alto ritorno):
  - Blend mode in composite_surface_at  [prerequisito per blur e 3D]
  - Motion blur completamento (curve, guard)
  - Color management primaries + range
  - Low-end policy engine
  - Time remap

Sprint 2 (media complessità):
  - Adjustment layers
  - Inter-frame precomp cache
  - Tiling ROI

Sprint 3 (alta complessità / dipendenze esterne):
  - 3D: depth buffer + luci
  - Text shaping (HarfBuzz)
  - Audio mux

Sprint 4 (lungo termine):
  - Shape extrusion
  - Ombre
  - Glyph animation
```

---

## Dipendenze esterne da aggiungere

| Feature | Dipendenza | vcpkg | apt |
|---------|-----------|-------|-----|
| Text shaping | FreeType | `freetype` | `libfreetype6-dev` |
| Text shaping | HarfBuzz | `harfbuzz` | `libharfbuzz-dev` |
| Audio mux | ffmpeg (già presente?) | `ffmpeg` | `libavcodec-dev` |

Per HarfBuzz: verificare se ffmpeg già linkato porta FreeType. In tal caso, solo HarfBuzz è nuovo.
