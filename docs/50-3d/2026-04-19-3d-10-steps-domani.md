# Tachyon 3D — 10 Passi per Domani Mattina

**Riferimento spec**: `docs/50-3d/2026-04-19-3d-rendering-implementation.md`  
**Ordine**: sequenziale — ogni passo dipende dal precedente  
**Build check**: dopo ogni passo fai girare il build + tutti i test esistenti prima di procedere

---

## Passo 1 — Estendi `EvaluatedLightState`

**File**: `include/tachyon/scene/evaluated_state.h`

Aggiungi i campi mancanti a `EvaluatedLightState`:
- `cone_angle_rad` (float, default ~0.698f ≈ 40°)
- `cone_feather_rad` (float, default ~0.175f ≈ 10°)
- `falloff_type` (std::string, default "smooth")
- `casts_shadows` (bool, default false)

Nessuna modifica alle strutture dati degli altri tipi ancora.

**Verifica**: build verde, zero test rotti.

---

## Passo 2 — Estendi `EvaluatedCameraState`

**File**: `include/tachyon/scene/evaluated_state.h`

Aggiungi a `EvaluatedCameraState`:
- `point_of_interest` (Vector3, default {0,0,0})
- `is_two_node` (bool, default true)
- `dof_enabled` (bool, default false)
- `focus_distance` (float, default 1000.0f)
- `aperture` (float, default 1.0f)
- `blur_level` (float, default 1.0f)

**Verifica**: build verde, zero test rotti.

---

## Passo 3 — Estendi `EvaluatedLayerState` + `MaterialOptions`

**File**: `include/tachyon/scene/evaluated_state.h`

Aggiungi a `EvaluatedLayerState`:
- Struttura interna `MaterialOptions { ambient_coeff, diffuse_coeff, specular_coeff, shininess, metal }`
- Campo `material` di tipo `MaterialOptions`
- `orientation_xyz_deg` (Vector3)
- `anchor_point_3d` (Vector3)
- `scale_3d` (Vector3, default {1,1,1})
- `world_normal` (Vector3, default {0,0,1})

Aggiorna il copy constructor e `operator=` per includere tutti i nuovi campi.

**Verifica**: build verde, tutti i test esistenti passano (inclusi quelli che costruiscono `EvaluatedLayerState`).

---

## Passo 4 — `DepthBuffer`

**File da creare**:
- `include/tachyon/renderer2d/depth_buffer.h`
- `src/renderer2d/depth_buffer.cpp`

Implementa la classe esattamente come da spec (sezione C.4):
- `DepthBuffer(int width, int height)`
- `void clear()`
- `bool test_and_write(int x, int y, float inv_z)`
- `float get(int x, int y) const`

Aggiungi `renderer2d/depth_buffer.cpp` a `src/CMakeLists.txt`.

**Test**: scrivi `tests/unit/renderer2d/3d_depth_buffer_tests.cpp` (3 test: clear, test_and_write primo pixel, depth test vince il più vicino). Aggiungili a `tests/CMakeLists.txt`.

**Verifica**: build verde, nuovi test passano.

---

## Passo 5 — `build_layer_world_matrix` nell'Evaluator

**File**: `src/scene/evaluator.cpp`

Aggiungi la funzione statica `build_layer_world_matrix` esattamente come da spec (sezione D.2).

Aggiungi `extract_world_normal` (sezione D.3).

Nel codice di `evaluate_layer()` (o equivalente), popola:
- `layer.world_matrix` — usando la funzione nuova (prima che esisteva solo il 2D transform)
- `layer.world_position3` — estrai dalla colonna di traslazione del world_matrix
- `layer.world_normal` — usa `extract_world_normal`
- `layer.orientation_xyz_deg`, `layer.anchor_point_3d`, `layer.scale_3d` — dal JSON (con fallback ai default se assenti)

**Test**: scrivi `tests/unit/renderer2d/3d_world_matrix_tests.cpp` (test identity, translation, rotation_x, anchor — vedi sezione J.1 della spec).

**Verifica**: build verde, tutti i test passano.

---

## Passo 6 — Popola Camera e Luci dall'Evaluator

**File**: `src/scene/evaluator.cpp`

Per i layer di tipo `Camera`:
- Leggi `zoom` dal JSON, chiama `zoom_to_fov_y(zoom, comp_height)` (sezione E.1 della spec)
- Popola `EvaluatedCameraState` con `is_two_node`, `point_of_interest`, DOF
- Costruisci `camera.camera` (fov_y_rad, aspect, near_z=1, far_z=10000)

Per i layer di tipo `Light`:
- Leggi `cone_angle`, `cone_feather` (in gradi → converti in rad), `falloff`, `casts_shadows`
- Popola `EvaluatedLightState` completo

**Verifica**: build verde. Scrivi un quick test che valuta una composizione con un layer camera e verifica che `fov_y_rad` sia ragionevole (> 0, < π).

---

## Passo 7 — Proiezione 3D in `EvaluatedCompositionRenderer`

**File**: `src/renderer2d/evaluated_composition_renderer.cpp`

Aggiungi la funzione `project_3d_layer()` (sezione H.2 della spec).

Prima del loop di compositing 2D:
1. Raccogli tutti i layer con `is_3d == true`
2. Costruisci view matrix (sezione E.2 — usa `is_two_node` e `point_of_interest`)
3. Costruisci projection matrix da `camera.camera`
4. Proietta ogni layer con `project_3d_layer()`
5. Filtra layer dietro la camera (inv_z == 0)
6. Ordina back-to-front (inv_z ascending)

**Verifica**: build verde. Non serve ancora che i layer si rendano — verifica solo che il sort non crashi con 0, 1 o N layer 3D.

---

## Passo 8 — Rendering 3D Layer come Sprite 2D + Lighting

**File**: `src/renderer2d/evaluated_composition_renderer.cpp`

Per ogni `ProjectedLayer` (back-to-front):
1. Prendi la surface del layer (già renderata dal pipeline 2D normale)
2. Applica scala prospettica (`scale_factor`) alla surface
3. Accumula contributi di tutte le luci (sezione F.1-F.3 della spec)
4. Applica tint RGB ai pixel della surface
5. Componi la surface a `(screen_x, screen_y)` nel frame corrente
6. Scrivi inv_z al depth buffer per ogni pixel coperto

Gestisci i casi limite della tabella sezione I (zoom≤0, layer dietro camera, matrice degenere).

**Test**: scrivi `tests/unit/renderer2d/3d_lighting_tests.cpp` (ambient only, parallel NdotL, falloff smooth, spot fuori cono).

**Verifica**: build verde, tutti i test passano.

---

## Passo 9 — Depth of Field (Post-Process)

**File**: `src/renderer2d/evaluated_composition_renderer.cpp`

Dopo aver composito tutti i 3D layer ma prima dei 2D layer:
1. Per ogni 3D layer già composito, calcola `dof_blur_radius()` (sezione G.1)
2. Se radius > 0.5px, applica separable gaussian blur alla sua surface prima della compositing
3. Layers a `focus_distance` ± epsilon non ricevono blur

Puoi riusare il gaussian esistente nel codebase (se presente) o implementarlo come two-pass (H + V) direttamente.

**Verifica**: build verde. Il test visivo è soggettivo ma il blur non deve crashare né produrre NaN/inf.

---

## Passo 10 — Test Visivi e Cleanup

1. Crea le scene di test visivo (sezione J.2 della spec) in `tests/visual/3d_*`
2. Rendi una scena di riferimento semplice: layer solido rosso a z=−300 sotto luce ambient bianca → PNG di riferimento
3. Aggiungi al test runner: confronto pixel-diff ≤ 2% contro PNG di riferimento
4. Controlla il perf budget (sezione K): misura il tempo per frame con 4 layer 3D + 4 luci
5. Fai un commit pulito: `"feat(3d): complete 3D rendering pipeline — depth sort, lighting, DOF"`

**Verifica finale**: build verde, tutti i test (unit + visual) passano, nessun memory leak (valgrind/asan se disponibile).

---

## Dipendenze tra i Passi

```
1 (EvaluatedLightState)
2 (EvaluatedCameraState)
3 (EvaluatedLayerState)
    └─> 4 (DepthBuffer) — indipendente dagli altri
    └─> 5 (world_matrix) — dipende da 3
        └─> 6 (evaluator camera/lights) — dipende da 1, 2, 5
            └─> 7 (proiezione) — dipende da 6
                └─> 8 (rendering + lighting) — dipende da 4, 7
                    └─> 9 (DOF) — dipende da 8
                        └─> 10 (test visivi + cleanup)
```

I passi 1-3 si possono fare in sequenza rapida nello stesso commit di "struct expansion".  
Il passo 4 (DepthBuffer) si può fare in parallelo con i passi 5-6 se lavori su branch separati.

---

*Buona fortuna domani mattina. Leggi l'intera spec prima di scrivere la prima riga di codice.*
