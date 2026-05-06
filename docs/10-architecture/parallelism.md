---
Status: Canonical
Last reviewed: 2026-05-06
Owner: Core Team
Supersedes: N/A
---

# Parallel Rendering

## Stato attuale

### Cosa esiste
| Componente | File | Stato |
|---|---|---|
| `TileGrid` + `build_tile_grid()` | `include/tachyon/runtime/execution/scheduling/tile_scheduler.h` | ✓ griglia tile 256px |
| `RenderBatchSpec` + `run_render_batch()` | `include/tachyon/runtime/execution/batch/batch_runner.h` | ✓ multi-job, worker_count |
| `RenderSession` | `include/tachyon/runtime/execution/session/render_session.h` | ✓ |
| `FrameCache` thread-safe (mutex) | `frame_cache.h` | ✓ |
| `SurfacePool` | `include/tachyon/runtime/resource/surface_pool.h` | ✓ |

### Gap principali
- **Frame parallelism**: i frame vengono renderizzati sequenzialmente — nessun thread pool per frame paralleli
- **Layer parallelism**: i layer di una composizione vengono processati in sequenza
- **Tile rendering**: `TileGrid` esiste ma non è usato nel loop di render
- **Memory budget**: nessun limite esplicito sulla RAM usata dal frame renderer durante il batch

---

## Architettura di parallelismo da implementare

### Livello 1 — Batch parallelism (più importante per YouTube automatico)

**Già disponibile via `RenderBatchSpec::worker_count`.**

```json
{
  "worker_count": 4,
  "jobs": [
    {"scene_path": "scene_a.json", "job_path": "job_a.json"},
    {"scene_path": "scene_b.json", "job_path": "job_b.json"},
    {"scene_path": "scene_c.json", "job_path": "job_c.json"},
    {"scene_path": "scene_d.json", "job_path": "job_d.json"}
  ]
}
```

**Verifica che `run_render_batch()` usi effettivamente worker_count thread.**
Se usa 1 worker in serie, è sufficiente aggiungere un thread pool:

```cpp
ResolutionResult<RenderBatchResult> run_render_batch(const RenderBatchSpec& spec, size_t worker_count) {
    BS::thread_pool pool(worker_count);  // o std::async
    std::vector<std::future<RenderBatchJobResult>> futures;
    
    for (const auto& item : spec.jobs) {
        futures.push_back(pool.submit([item]() {
            return run_single_job(item);
        }));
    }
    
    RenderBatchResult result;
    for (auto& f : futures) {
        auto job_result = f.get();
        result.jobs.push_back(job_result);
        if (job_result.success) result.succeeded++;
        else result.failed++;
    }
    return {result};
}
```

**Attenzione:** ogni job deve avere il proprio `FrameCache`, `MediaManager` e `FontRegistry` — nessuno stato condiviso tra job.

---

### Livello 2 — Frame parallelism (entro un singolo video)

Renderizzare frame N e N+1 in parallelo. **Funziona solo se non ci sono dipendenze inter-frame** (es. motion blur multi-frame, temporal denoising).

```cpp
// In RenderSession::render_frames():
const int lookahead = std::min(thread_count, 4);  // max 4 frame in parallel
BS::thread_pool pool(lookahead);
std::queue<std::future<RenderedFrame>> queue;

for (int64_t frame = start; frame <= end; ++frame) {
    // Mantieni max `lookahead` frame in volo
    while (queue.size() >= lookahead) {
        auto rendered = queue.front().get();
        queue.pop();
        encoder.encode(rendered);
    }
    queue.push(pool.submit([frame, &scene]() {
        return render_single_frame(scene, frame);
    }));
}
// Drain
while (!queue.empty()) {
    encoder.encode(queue.front().get());
    queue.pop();
}
```

**Vincoli:**
- `FrameCache` deve essere thread-safe (già ha mutex)
- `MediaManager` deve avere un pool di `VideoDecoder` per frame (già ha pool)
- L'encoder deve ricevere i frame **in ordine** — il queue garantisce questo

---

### Livello 3 — Tile rendering (per frame ad alta risoluzione)

`TileGrid` esiste. L'obiettivo è dividere un frame 4K in tile e renderizzarle in parallelo.

```cpp
TileGrid grid = build_tile_grid(composition_state, 256);

BS::thread_pool pool(std::thread::hardware_concurrency());
std::vector<std::future<void>> tile_futures;

for (const auto& tile_rect : grid.tiles) {
    tile_futures.push_back(pool.submit([&, tile_rect]() {
        render_tile(composition_state, tile_rect, output_framebuffer);
    }));
}
for (auto& f : tile_futures) f.get();
```

**Vincoli:**
- I layer devono essere renderizzati per ogni tile indipendentemente, o:
- Renderi tutti i layer a dimensione piena e poi compositi per tile (più semplice ma meno efficiente)
- Il compositor deve usare `clip` per limitare il lavoro al tile corrente

**Approccio consigliato per prima implementazione:** renderizza l'intera composizione una volta sola (come ora), poi parallellizza solo la fase di encoding/downscale per output.

---

### Livello 4 — Layer parallelism (avanzato)

I layer di una composizione sono dipendenti (compositing in ordine). Non si parallelizzano direttamente. Si può parallelizzare:

1. **Rasterizzazione pre-compositor**: ogni layer viene rasterizzato nel proprio buffer prima del compositing. La rasterizzazione di layer indipendenti (senza dipendenze matte) è parallelizzabile.

2. **Precomp rendering**: le precomp nested possono essere renderizzate in parallelo prima del compositor principale.

```cpp
// Analisi dipendenze (dal DependencyGraph esistente):
auto independent_layers = dependency_graph.get_independent_groups(layers);

for (auto& group : independent_layers) {
    // Tutti i layer nel gruppo non si dipendono tra loro → parallelo
    pool.submit_group(group, [](LayerSpec& layer) {
        return rasterize_layer(layer);
    }).wait();
}
```

---

## Memory budget per batch

Quando si renderizzano N video in parallelo, la RAM può esplodere.

```cpp
struct BatchMemoryPolicy {
    size_t max_total_ram_bytes{4ULL * 1024 * 1024 * 1024};  // 4GB
    size_t per_job_frame_cache_bytes{512ULL * 1024 * 1024}; // 512MB per job
    size_t per_job_asset_cache_bytes{1ULL * 1024 * 1024 * 1024}; // 1GB per job
};
```

Prima di lanciare ogni job, verifica che la RAM stimata sia disponibile. Se non lo è, metti il job in attesa.

---

## Ordine di implementazione

```
1. Verificare che run_render_batch() usi worker_count thread reali
2. Frame parallelism con lookahead queue (4 frame in volo)
3. Memory budget per job nel batch runner
4. Tile rendering: usare TileGrid esistente nel compositor
5. Layer parallelism per precomp nested (usa DependencyGraph esistente)
```

Per YouTube automatico (20+ video in batch), item 1 e 2 danno il 90% del beneficio.

---

## CLI batch

```bash
# Renderizza 4 video in parallelo
tachyon batch render_batch.json --workers 4

# Oppure con override output
tachyon batch render_batch.json --workers 4 --output-dir /output/
```

`parse_render_batch_file()` esiste già. Serve solo esporre `--workers` come argomento CLI.
