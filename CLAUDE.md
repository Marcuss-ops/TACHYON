# Tachyon — Regole architetturali

## Filosofia

Tachyon è un motore C++ headless per motion graphics deterministico, analogo ad After Effects
senza UI. L'obiettivo è produrre video di qualità professionale su CPU a bassa fascia.

---

## Regola fondamentale: il pattern Presets

**Tutto il contenuto riusabile segue esattamente questo schema:**

```
Params  →  Builder  →  SceneSpec / LayerSpec
```

Ogni dominio ha tre strati, niente di più:

1. `XParams : LayerParams` — dati con default ragionevoli
2. `build_x_*(const XParams& p) → LayerSpec` — una funzione per variante
3. `build_x_scene(...)` — scena completa quando serve

### File per ogni dominio

```
include/tachyon/presets/<domain>/
    <domain>_params.h      ← XParams : LayerParams
    <domain>_builders.h    ← dichiarazioni build_x_*()

src/presets/<domain>/
    <domain>_builders.cpp  ← implementazioni
```

### Domini approvati

| Dominio       | Params               | Output              | Stato     |
|---------------|----------------------|---------------------|-----------|
| `text`        | `TextParams`         | `LayerSpec`         | ✅ fatto  |
| `image`       | `ImageParams`        | `LayerSpec`         | skeleton  |
| `transition`  | `TransitionParams`   | `LayerTransitionSpec` | skeleton |
| `counter`     | `CounterParams`      | `LayerSpec`         | skeleton  |
| `background`  | (già in procedural.h) | `LayerSpec`        | ✅ fatto  |
| `shape`       | `ShapeParams`        | `LayerSpec`         | todo      |
| `audio`       | `AudioParams`        | `AudioTrackSpec`    | todo      |

---

## Cosa NON fare

- **Non creare `XOptions` / `XSceneOptions`** — si usa `XParams : LayerParams`
- **Non creare singleton registry per ogni dominio** — il dispatcher è in `build_x()`
- **Non mettere logica di costruzione nei header** — solo in `.cpp`
- **Non duplicare campi di `LayerParams`** in struct derivate (in_point, out_point, x, y, w, h, opacity)
- **Non aggiungere file JSON in `scenes/`** come sorgenti — i JSON sono solo output/debug/cache

## Base condivisa

`include/tachyon/presets/preset_base.h` definisce `LayerParams` e `SceneParams`.
Ogni nuova struct Params **deve** ereditare da `LayerParams`.

```cpp
struct MyParams : LayerParams {
    // solo campi specifici del dominio
};
```

## Primitivi di animazione testo

`include/tachyon/text/animation/text_presets.h` contiene i 20 `make_*_animator()` che
restituiscono `TextAnimatorSpec`. Sono **primitivi interni** — non vanno usati direttamente
dall'utente finale, solo dai builder in `src/presets/text/text_builders.cpp`.

## Umbrella header

`include/tachyon/presets/presets.h` è l'unico punto di ingresso pubblico.
Aggiungi qui ogni nuovo dominio appena il suo header è pronto.

## Aggiungere un nuovo dominio — checklist

1. Crea `include/tachyon/presets/<domain>/<domain>_params.h` con `struct XParams : LayerParams`
2. Crea `include/tachyon/presets/<domain>/<domain>_builders.h` con le dichiarazioni
3. Crea `src/presets/<domain>/<domain>_builders.cpp` con un `make_x_layer_base()` privato
4. Aggiungi `presets/<domain>/<domain>_builders.cpp` in `src/CMakeLists.txt`
5. Includi il nuovo header in `include/tachyon/presets/presets.h`
6. Scrivi test in `tests/unit/presets/<domain>_contract_tests.cpp`
