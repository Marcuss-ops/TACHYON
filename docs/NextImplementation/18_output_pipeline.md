# Output Pipeline

## Stato attuale

### Cosa esiste
| Componente | File | Stato |
|---|---|---|
| `OutputProfile` con codec, pixel_format, rate_control, CRF | `render_job.h` | ✓ |
| `OutputFormat`: Video, Gif, ImageSequence, ProRes, WebM | `render_job.h` | ✓ enum |
| `AudioEncoder` AAC | `include/tachyon/audio/io/audio_encoder.h` | ✓ FFmpeg |
| `AudioExporter` multi-track | `include/tachyon/audio/io/audio_export.h` | ✓ |
| `OutputColorProfile` (space, transfer, range) | `render_job.h` | ✓ |
| `OutputVideoProfile` (codec, pixel_format, rate_control, crf) | `render_job.h` | ✓ |
| `OutputBufferingProfile` (strategy, max_frames_in_queue) | `render_job.h` | ✓ |

### Gap principali
- **Nessun preset named** (YouTube 1080p, Shorts, 4K, etc.) — i parametri vanno tutti specificati a mano
- **PNG sequence** è nell'enum ma l'implementazione è da verificare
- **Thumbnail export** non esiste
- **H.265 / HEVC** non è esplicitamente supportato come opzione semplice
- **Metadata YouTube** (title, description, tags) non è scrivibile nei container
- Nessun **progress reporting** strutturato durante l'encode

---

## Preset YouTube (cosa aggiungere)

### Struttura preset

```cpp
// include/tachyon/runtime/execution/jobs/output_presets.h

struct OutputPreset {
    std::string name;
    OutputProfile profile;
};

const OutputPreset& get_preset(const std::string& name);
std::vector<std::string> list_presets();
```

### Preset da implementare

**YouTube 1080p 30fps (standard):**
```json
{
  "name": "youtube_1080p_30",
  "container": "mp4",
  "format": "video",
  "video": {
    "codec": "libx264",
    "pixel_format": "yuv420p",
    "rate_control_mode": "crf",
    "crf": 18,
    "preset": "slow",
    "profile": "high",
    "level": "4.0",
    "keyframe_interval": 30
  },
  "audio": {
    "codec": "aac",
    "sample_rate": 48000,
    "channels": 2,
    "bitrate_kbps": 192
  },
  "color": {
    "space": "bt709",
    "transfer": "bt709",
    "range": "tv"
  }
}
```

**YouTube 1080p 60fps:**
```json
{
  "name": "youtube_1080p_60",
  "container": "mp4",
  "video": {
    "codec": "libx264",
    "pixel_format": "yuv420p",
    "rate_control_mode": "crf",
    "crf": 16,
    "preset": "slow",
    "profile": "high",
    "level": "4.2"
  }
}
```

**YouTube Shorts 1080x1920:**
```json
{
  "name": "youtube_shorts",
  "container": "mp4",
  "video": {
    "codec": "libx264",
    "pixel_format": "yuv420p",
    "rate_control_mode": "crf",
    "crf": 18
  },
  "resolution": {"width": 1080, "height": 1920}
}
```

**YouTube 4K:**
```json
{
  "name": "youtube_4k",
  "container": "mp4",
  "video": {
    "codec": "libx264",
    "pixel_format": "yuv420p",
    "rate_control_mode": "crf",
    "crf": 15,
    "preset": "slower"
  },
  "resolution": {"width": 3840, "height": 2160}
}
```

**H.265 (file più piccoli, stessa qualità):**
```json
{
  "name": "youtube_1080p_h265",
  "container": "mp4",
  "video": {
    "codec": "libx265",
    "pixel_format": "yuv420p",
    "rate_control_mode": "crf",
    "crf": 20,
    "preset": "slow"
  }
}
```

**PNG sequence (per compositing esterno):**
```json
{
  "name": "png_sequence",
  "container": "png",
  "format": "image_sequence",
  "video": {
    "codec": "png",
    "pixel_format": "rgba"
  }
}
```

**ProRes 4444 (qualità massima):**
```json
{
  "name": "prores_4444",
  "container": "mov",
  "format": "prores",
  "video": {
    "codec": "prores_ks",
    "pixel_format": "yuva444p10le",
    "profile": 4
  }
}
```

---

## Uso dei preset nel render job

```json
{
  "job_id": "render_001",
  "scene_ref": "scene.json",
  "composition_target": "main",
  "frame_range": {"start": 0, "end": 899},
  "output": {
    "destination": {"path": "output/video.mp4"},
    "preset": "youtube_1080p_30"
  }
}
```

Il resolver carica il preset e lo fonde con eventuali override:
```cpp
OutputProfile resolve_output_profile(const OutputContract& contract) {
    if (!contract.preset_name.empty()) {
        auto profile = get_preset(contract.preset_name).profile;
        // Applica override espliciti sopra il preset
        if (contract.profile.video.crf.has_value())
            profile.video.crf = contract.profile.video.crf;
        return profile;
    }
    return contract.profile;
}
```

---

## Thumbnail export

Per ogni render job, opzione di esportare il frame N come PNG:

```json
{
  "thumbnail": {
    "enabled": true,
    "frame": 90,
    "path": "output/thumbnail.jpg",
    "width": 1280,
    "height": 720,
    "quality": 95
  }
}
```

Implementazione nel `RenderSession`:
```cpp
if (job.thumbnail.enabled) {
    auto frame = render_single_frame(scene, job.thumbnail.frame);
    auto resized = resize_surface(frame, job.thumbnail.width, job.thumbnail.height);
    save_jpeg(resized, job.thumbnail.path, job.thumbnail.quality);
}
```

---

## Progress reporting

Aggiungere callback strutturato durante l'encode:

```cpp
struct RenderProgress {
    int64_t frames_done;
    int64_t frames_total;
    double elapsed_seconds;
    double estimated_remaining_seconds;
    double fps;
};

using ProgressCallback = std::function<void(const RenderProgress&)>;

// In RenderSession o RenderJob:
void set_progress_callback(ProgressCallback cb);
```

Chiamata ogni N frame (es. ogni 10):
```cpp
if (frame_number % 10 == 0 && m_progress_cb) {
    m_progress_cb(RenderProgress{
        .frames_done = frame_number - start,
        .frames_total = end - start,
        .elapsed_seconds = elapsed(),
        .estimated_remaining_seconds = estimate_remaining(),
        .fps = frames_done / elapsed_seconds
    });
}
```

---

## Metadata container

Per YouTube, i metadata del video si scrivono nel container MP4:

```json
{
  "metadata": {
    "title": "Il mio video YouTube",
    "artist": "Tachyon Studio",
    "date": "2025-01-01",
    "comment": "Generato automaticamente"
  }
}
```

Implementazione via FFmpeg `av_dict_set`:
```cpp
if (!job.metadata.empty()) {
    for (const auto& [key, value] : job.metadata) {
        av_dict_set(&format_ctx->metadata, key.c_str(), value.c_str(), 0);
    }
}
```

---

## CLI

```bash
# Con preset named
tachyon render scene.json job.json --preset youtube_1080p_30

# Lista preset disponibili
tachyon presets list

# Info su preset
tachyon presets info youtube_4k

# Export solo thumbnail
tachyon thumbnail scene.json --frame 90 --out thumb.jpg --width 1280 --height 720

# Batch con preset
tachyon batch batch.json --preset youtube_1080p_30 --workers 4
```

---

## Ordine di implementazione

```
1. OutputPreset struct + file JSON preset (youtube_1080p_30, shorts, 4k, h265, png, prores)
2. Resolver preset nel RenderJob parser
3. "preset" field nel job JSON
4. Thumbnail export (render_single_frame già esiste, serve save_jpeg)
5. Progress callback in RenderSession
6. Metadata container via FFmpeg av_dict
7. CLI: tachyon presets list/info
8. H.265 test e validazione parametri
```

Item 1+2+3 sbloccano i preset named — il beneficio maggiore per produzione automatica.

