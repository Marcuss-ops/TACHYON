# Scene Validator

## Stato attuale

### Cosa esiste
| Componente | File | Stato |
|---|---|---|
| `validate_render_job()` | `src/runtime/execution/render_job/render_job_validate.cpp` | ✓ base |
| CLI `tachyon validate` | `src/core/cli/cli_validate.cpp` | ✓ |
| `AssetReport` + `build_asset_report()` | `include/tachyon/media/resolution/asset_resolution.h` | ✓ |
| `FontPreloader` con validation | `include/tachyon/text/fonts/management/font_preloader.h` | ✓ |
| `FontCoverageReporter` | `include/tachyon/text/fonts/utils/font_coverage_reporter.h` | ✓ |
| `DiagnosticBag` + `ResolutionResult` | `runtime/core/diagnostics/diagnostics.h` | ✓ |

### Gap principali
- Nessuna stima della RAM necessaria pre-render
- Nessun controllo safe area (testo/elementi fuori dai margini sicuri YouTube)
- Nessun controllo "layer senza duration" (out_point == in_point)
- Nessuna validazione degli effetti (tipo non registrato nell'EffectHost)
- Le validation esistenti sono sparse — manca una facade unificata `SceneValidator`

---

## Architettura target: `SceneValidator`

Un singolo punto di ingresso che orchestra tutti i check pre-render:

```cpp
// include/tachyon/core/validation/scene_validator.h

struct ValidationWarning {
    std::string code;    // "font.missing", "asset.missing", "text.safe_area", ...
    std::string message;
    std::string location; // "layers[title].font_id"
};

struct ValidationError {
    std::string code;
    std::string message;
    std::string location;
};

struct SceneValidationReport {
    std::vector<ValidationError>   errors;
    std::vector<ValidationWarning> warnings;

    // Stime risorse
    size_t estimated_ram_bytes{0};
    size_t estimated_vram_bytes{0};
    double estimated_render_time_seconds{0.0};

    bool passed() const { return errors.empty(); }

    // Summary human-readable
    std::string format_summary() const;
};

class SceneValidator {
public:
    explicit SceneValidator(const std::filesystem::path& root_dir);

    SceneValidationReport validate(const SceneSpec& scene, const RenderJob& job) const;

private:
    // Check individuali — ognuno aggiunge a errors/warnings
    void check_assets(const SceneSpec&, SceneValidationReport&) const;
    void check_fonts(const SceneSpec&, SceneValidationReport&) const;
    void check_audio(const SceneSpec&, SceneValidationReport&) const;
    void check_layers(const SceneSpec&, SceneValidationReport&) const;
    void check_effects(const SceneSpec&, SceneValidationReport&) const;
    void check_safe_area(const SceneSpec&, SceneValidationReport&) const;
    void check_memory(const SceneSpec&, const RenderJob&, SceneValidationReport&) const;

    std::filesystem::path m_root;
};
```

---

## Check 1 — Assets (già parziale)

Usa `build_asset_report()` esistente:
```cpp
void SceneValidator::check_assets(const SceneSpec& scene, SceneValidationReport& report) const {
    auto asset_report = build_asset_report(scene, m_root);
    for (const auto& entry : asset_report.entries) {
        if (!entry.exists) {
            report.errors.push_back({
                "asset.missing",
                entry.error,
                "assets[" + entry.path.string() + "]"
            });
        }
    }
    // Summary
    // "12 immagini, 3 video, 1 voiceover, 2 font, 1 musica, 4 sound effects"
    // → già calcolato da asset_report.image_count, video_count, etc.
}
```

---

## Check 2 — Fonts

Usa `FontPreloader` + `FontCoverageReporter` esistenti:
```cpp
void SceneValidator::check_fonts(const SceneSpec& scene, SceneValidationReport& report) const {
    if (!scene.font_manifest_path.has_value()) {
        // Controlla che ogni font_id nei layer esista nel FontRegistry
        for (const auto& comp : scene.compositions) {
            for (const auto& layer : comp.layers) {
                if (layer.type == "text" && !layer.font_id.empty()) {
                    if (!font_registry_has(layer.font_id)) {
                        report.errors.push_back({"font.missing",
                            "Font '" + layer.font_id + "' non trovato",
                            "layers[" + layer.id + "].font_id"});
                    }
                }
            }
        }
        return;
    }
    // Con manifest: usa FontPreloader
    FontPreloader preloader(resolver);
    auto result = preloader.preload_from_file(*scene.font_manifest_path);
    for (const auto& missing : result.missing_fonts) {
        report.errors.push_back({"font.missing", "Font non trovato: " + missing, "font_manifest"});
    }
}
```

---

## Check 3 — Layers

```cpp
void SceneValidator::check_layers(const SceneSpec& scene, SceneValidationReport& report) const {
    for (const auto& comp : scene.compositions) {
        for (const auto& layer : comp.layers) {
            const std::string loc = "compositions[" + comp.id + "].layers[" + layer.id + "]";

            // Layer senza duration
            if (layer.out_point <= layer.in_point) {
                report.errors.push_back({"layer.no_duration",
                    "Layer '" + layer.id + "' ha out_point <= in_point", loc});
            }

            // Precomp reference mancante
            if (layer.type == "precomp" && layer.precomp_id.has_value()) {
                bool found = false;
                for (const auto& c : scene.compositions) {
                    if (c.id == *layer.precomp_id) { found = true; break; }
                }
                if (!found) {
                    report.errors.push_back({"precomp.missing",
                        "Precomp '" + *layer.precomp_id + "' non trovato", loc});
                }
            }

            // Track matte reference mancante
            if (layer.track_matte_layer_id.has_value()) {
                if (!find_layer_in_comp(comp, *layer.track_matte_layer_id)) {
                    report.errors.push_back({"matte.missing",
                        "Matte layer '" + *layer.track_matte_layer_id + "' non trovato", loc});
                }
            }

            // Testo vuoto
            if (layer.type == "text" && layer.text_content.empty()) {
                report.warnings.push_back({"text.empty",
                    "Layer testo '" + layer.id + "' ha testo vuoto", loc});
            }
        }
    }
}
```

---

## Check 4 — Safe area

YouTube consiglia margini di 10% per thumbnail e UI overlay. Area sicura per 1920x1080 = `[192, 108, 1728, 972]`.

```cpp
void SceneValidator::check_safe_area(const SceneSpec& scene, SceneValidationReport& report) const {
    for (const auto& comp : scene.compositions) {
        const float margin_x = comp.width * 0.10f;
        const float margin_y = comp.height * 0.10f;
        const RectF safe = {margin_x, margin_y,
                            comp.width - margin_x * 2,
                            comp.height - margin_y * 2};

        for (const auto& layer : comp.layers) {
            if (layer.type != "text") continue;
            // Stima bounds testo dalla position/size/font_size
            auto bounds = estimate_text_bounds(layer);
            if (!safe.contains(bounds)) {
                report.warnings.push_back({"text.safe_area",
                    "Layer '" + layer.id + "' potrebbe uscire dalla safe area YouTube",
                    "compositions[" + comp.id + "].layers[" + layer.id + "]"});
            }
        }
    }
}
```

---

## Check 5 — Effects

```cpp
void SceneValidator::check_effects(const SceneSpec& scene, SceneValidationReport& report) const {
    const auto& host = builtin_effect_host();
    for (const auto& comp : scene.compositions) {
        for (const auto& layer : comp.layers) {
            for (const auto& effect : layer.effects) {
                if (!host.has_effect(effect.type)) {
                    report.errors.push_back({"effect.unknown",
                        "Effetto '" + effect.type + "' non registrato",
                        "layers[" + layer.id + "].effects"});
                }
            }
        }
    }
}
```

---

## Check 6 — Memory estimate

```cpp
void SceneValidator::check_memory(const SceneSpec& scene, const RenderJob& job,
                                   SceneValidationReport& report) const {
    // Frame size in bytes (RGBA float = 4 * 4 bytes per pixel)
    const size_t w = scene.compositions[0].width;
    const size_t h = scene.compositions[0].height;
    const size_t frame_bytes = w * h * 4 * 4;  // RGBA 32-bit float

    // Stima: 1 frame per layer + 2 compositing buffer + output
    size_t max_layers = 0;
    for (const auto& comp : scene.compositions) {
        max_layers = std::max(max_layers, comp.layers.size());
    }
    const size_t estimated = frame_bytes * (max_layers + 2);

    report.estimated_ram_bytes = estimated;

    const size_t available = get_available_ram_bytes();
    if (estimated > available * 0.8) {
        report.warnings.push_back({"memory.high",
            "RAM stimata " + format_bytes(estimated) + " supera l'80% della disponibile",
            "render_job"});
    }
}
```

---

## CLI output

```bash
tachyon validate scene.json job.json
```

Output:
```
TACHYON Scene Validator
=======================
Assets:   12 images ✓  3 videos ✓  1 voiceover ✓  2 fonts ✓
Layers:   47 layers — 0 errors, 2 warnings
Effects:  all registered ✓
Safe area: 3 text layers outside margins (WARNING)
Memory:   estimated 1.2 GB — OK

ERRORS (0):   none
WARNINGS (3):
  [text.safe_area] layers[lower_third]: potrebbe uscire dalla safe area YouTube
  [text.safe_area] layers[ticker]: ...
  [memory.high]: RAM stimata 3.8 GB supera 80% disponibile

Result: PASSED WITH WARNINGS
```

---

## Integrazione nel render pipeline

```cpp
// In RenderJob execution, prima del render:
SceneValidator validator(root_dir);
auto report = validator.validate(scene, job);

if (!report.passed()) {
    log_errors(report.errors);
    return {false, "Validation failed"};
}
if (!report.warnings.empty()) {
    log_warnings(report.warnings);  // non blocca
}
// Procedi con il render
```

---

## Ordine di implementazione

```
1. SceneValidator facade con check_assets() (usa build_asset_report esistente)
2. check_fonts() (usa FontPreloader esistente)
3. check_layers() (duration, precomp ref, matte ref)
4. check_effects() (usa EffectHost::has_effect)
5. check_memory() (stima semplice)
6. check_safe_area() (geometria statica)
7. CLI output formattato
8. Integrazione nel render pipeline come gate obbligatorio
```

