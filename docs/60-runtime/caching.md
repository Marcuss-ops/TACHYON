# Render Cache

## Stato attuale

### Cosa esiste
| Componente | File | Stato |
|---|---|---|
| `FrameCache` multi-livello | `include/tachyon/runtime/cache/frame_cache.h` | ✓ property/layer/composition/frame |
| LRU eviction + budget_bytes (default 1GB) | `FrameCache` | ✓ |
| `CacheKeyBuilder` | `include/tachyon/runtime/cache/cache_key_builder.h` | ✓ |
| `FrameHasher` | `include/tachyon/runtime/execution/frames/frame_hasher.h` | ✓ |
| `DiskCache` | `include/tachyon/runtime/cache/disk_cache.h` | ✓ |
| Glyph cache in `FontInstance` | `include/tachyon/text/fonts/font_instance.h` | ✓ per size |
| Shaping cache | `include/tachyon/text/core/low_level/shaping/shaping_cache.h` | ✓ |
| Asset cache LRU | `CacheManager` in `asset_manager.h` | ✓ fino a 10GB |

### Gap principali
- Nessuna **static layer detection** automatica — ogni layer viene re-evaluato ogni frame anche se non cambia
- La **precomp cache** non è esplicita: composizioni nested vengono re-renderizzate anche se statiche
- `FontFace::content_hash()` non è incluso nella chiave del frame cache
- Non esiste una **text layout cache** — il layout viene ricalcolato ogni frame anche se testo e parametri non cambiano
- Nessuna **effect cache** — gli effetti statici (blur, LUT) vengono applicati ogni frame

---

## Architettura `FrameCache` esistente

```
FrameCache
├── property cache  — double per property hash
├── layer cache     — EvaluatedLayerState per layer hash
├── composition cache — EvaluatedCompositionState per composition hash
└── frame cache     — Framebuffer per FrameCacheKey
```

I 4 livelli esistono. Il problema è che il **hash input** non è sempre calcolato correttamente o usato dal caller.

---

## Gap 1 — Static layer detection

Un layer è statico se:
- Nessuna proprietà animata (tutti i keyframe sono a 0 o 1 keyframe costante)
- Nessuna espressione con `t`
- Nessun effetto time-varying

**Come implementarlo:**

```cpp
// In CompiledLayer (o LayerSpec):
bool is_static{false};  // calcolato dal SceneCompiler una volta

// SceneCompiler::analyze_static_layers():
bool layer_is_static(const LayerSpec& layer) {
    // Controlla ogni AnimatedScalarSpec — se tutti hanno 0 o 1 keyframe senza expression
    if (has_time_varying_keyframes(layer.opacity_property)) return false;
    if (has_time_varying_keyframes(layer.transform.position_x)) return false;
    // ... tutti i campi animati
    if (!layer.effects.empty()) return false;  // effetti potrebbero essere time-varying
    return true;
}
```

Se `is_static == true`, il layer viene renderizzato al frame 0 e il Framebuffer viene riusato per tutti i frame successivi.

---

## Gap 2 — Precomp cache

Una composizione nested (precomp) con layer tutti statici può essere renderizzata una volta e cachata.

```cpp
// Nella composition evaluation, prima di renderizzare:
if (composition_state.is_fully_static) {
    auto cached = frame_cache.lookup_composition(composition_hash);
    if (cached) return *cached;
}

// Renderizza e salva
auto result = render_composition(composition_state, ...);
if (composition_state.is_fully_static) {
    frame_cache.store_composition(composition_hash, result);
}
```

`is_fully_static` si determina al compile time dal `SceneCompiler`.

---

## Gap 3 — Text layout cache

Il layout del testo (word break, glyph positioning, shaping) è costoso e cambia solo se cambia testo, font, dimensione, wrapping o tracking.

```cpp
// Cache key per il layout:
struct TextLayoutCacheKey {
    std::string text;
    std::string font_family;
    uint32_t weight;
    uint32_t pixel_size;
    uint32_t box_width;
    uint32_t box_height;
    float tracking;
    bool word_wrap;
};

// Singleton o campo di RenderContext:
std::unordered_map<TextLayoutCacheKey, TextLayoutResult, ...> text_layout_cache;
```

La shaping cache esiste già (`ShapingCache`) ma non copre il layout completo. Aggiungere la cache layout sopra la shaping cache.

---

## Gap 4 — Font content_hash nel frame key

`FontFace::content_hash()` esiste ma non è incluso nella `FrameCacheKey`. Se si sostituisce il file TTF a parità di path, il cache non si invalida.

**Aggiungere a `FrameCacheKey`:**
```cpp
// In include/tachyon/runtime/execution/frames/frame_hasher.h
// o in CacheKeyBuilder:
void add_font_hash(uint64_t content_hash);
```

E nel ciclo di render, per ogni font usato dalla composizione:
```cpp
key_builder.add_font_hash(font_face->content_hash());
```

---

## Gap 5 — Effect cache

Effetti come `LUT`, `GaussianBlur` con parametri fissi, `Vignette` statico — non cambiano frame per frame.

```cpp
struct EffectCacheKey {
    std::string effect_type;
    std::string layer_id;
    uint64_t params_hash;  // hash di tutti i scalars/colors/strings
    uint32_t frame_number; // solo per effetti time-varying
};

// Se l'effetto non ha dipendenze temporali:
auto cached_surface = effect_cache.lookup(key);
if (cached_surface) return *cached_surface;

auto result = host.apply(effect.type, input, params);
effect_cache.store(key, result);
```

---

## Regola generale da applicare

```
se input_hash(frame N) == input_hash(frame N-1) → riusa output
```

Questo si applica a ogni livello:
- **Proprietà**: hash degli AnimatedScalarSpec + t → se uguale, riusa il double
- **Layer**: hash di tutte le proprietà evaluate + asset hash → se uguale, riusa EvaluatedLayerState
- **Composizione**: hash di tutti i layer evaluate → se uguale, riusa il Framebuffer
- **Frame finale**: hash della scena + frame number → se uguale, skippa il render

---

## Background statico + testo animato (caso d'uso principale)

```
Frame 1: render BG (lento) + render TEXT → cache BG hash
Frame 2: BG hash identico → skip BG render, leggi da cache
         TEXT hash cambiato → re-render solo TEXT
         Compositing: BG cached + TEXT nuovo
```

Risparmio tipico: 60-80% del tempo di render per scene con background statico.

---

## Ordine di implementazione

```
1. Static layer detection nel SceneCompiler (analisi keyframe + expressions)
2. Font content_hash nella FrameCacheKey
3. Text layout cache (sopra ShapingCache esistente)
4. Precomp cache: skip render se composition_hash in FrameCache
5. Effect cache per effetti con parametri statici
6. Metriche hit/miss esposte nel render log (già parzialmente in FrameCache)
```
