# Runtime API Contract

> **Status**: Draft
> **Scope**: Defines the stable contract between TACHYON runtime and external consumers
> (CLI, HTTP API, PipelineGen, n8n, Gradio, etc.)

---

## Guiding Principles

1. **Single Runtime Facade** - All interfaces (CLI, HTTP API, tests) must use the same internal C++ facade
2. **Deterministic** - Same input produces same output (given same seed and assets)
3. **Machine-Readable Errors** - All errors return structured JSON, not just text
4. **Stateless Operations** - Each render/validate/preview call is independent
5. **Asset Resolution** - Assets are resolved at render time, not at parse time

---

## Core Requests

### RenderRequest

```json
{
  "scene": "scene.cpp",
  "output": "output/video.mp4",
  "output_preset": "youtube_shorts",
  "quality": "draft",
  "frames": "0-300",
  "seed": 12345,
  "workers": 4,
  "memory_budget_mb": 2048
}
```

**Fields**:

| Field | Type | Required | Default | Description |
|-------|------|----------|---------|-------------|
| `scene` | string | Yes* | - | Path to C++ scene file or preset ID |
| `preset` | string | Yes* | - | Alternative to scene (use preset directly) |
| `output` | string | Yes | - | Output file path |
| `output_preset` | string | No | "preview" | Output preset name (youtube_1080p, youtube_shorts, etc.) |
| `quality` | string | No | "draft" | Quality tier: draft, final, max |
| `frames` | string | No | "0-0" (all) | Frame range like "0-300" or "100-" |
| `seed` | integer | No | 0 | Random seed for deterministic rendering |
| `workers` | integer | No | 1 | Number of worker threads |
| `memory_budget_mb` | integer | No | 0 (auto) | Memory budget in MB |

*Either `scene` or `preset` must be provided.

---

### PreviewRequest

```json
{
  "scene": "scene.cpp",
  "output": "output/preview.png",
  "frame": 90,
  "quality": "draft"
}
```

**Fields**:

| Field | Type | Required | Default | Description |
|-------|------|----------|---------|-------------|
| `scene` | string | Yes* | - | Path to C++ scene file or preset ID |
| `preset` | string | Yes* | - | Alternative to scene |
| `output` | string | No | auto | Output path (auto-generated if omitted) |
| `frame` | integer | No | 90 | Frame number to render |
| `quality` | string | No | "draft" | Quality tier |

---

### ValidateRequest

```json
{
  "scene": "scene.cpp",
  "check_assets": true
}
```

**Fields**:

| Field | Type | Required | Default | Description |
|-------|------|----------|---------|-------------|
| `scene` | string | Yes | - | Path to C++ scene file or preset ID |
| `check_assets` | boolean | No | true | Validate all referenced assets exist |

---

### ThumbnailRequest

```json
{
  "scene": "scene.cpp",
  "output": "output/thumb.jpg",
  "at": "3s",
  "frame": 90
}
```

**Fields**:

| Field | Type | Required | Default | Description |
|-------|------|----------|---------|-------------|
| `scene` | string | Yes | - | Path to C++ scene file or preset ID |
| `output` | string | No | auto | Output path (auto-generated if omitted) |
| `at` | string | No | - | Time offset like "3s", "00:00:03" |
| `frame` | integer | No | 90 | Frame number (overridden by `at` if provided) |

---

## Response Format

### Success Response

```json
{
  "ok": true,
  "data": {
    "job_id": "render_abc123",
    "status": "completed",
    "output_path": "output/video.mp4",
    "frames_rendered": 300,
    "duration_seconds": 10.5,
    "metadata": {
      "codec": "h264",
      "resolution": "1080x1920",
      "fps": 30
    }
  }
}
```

### Error Response

```json
{
  "ok": false,
  "error": {
    "code": "cli.out_missing",
    "message": "Missing value for --out",
    "hint": "Use --out output/video.mp4"
  }
}
```

**Error Codes**:

| Code | Meaning |
|------|---------|
| `scene.not_found` | Scene file or preset not found |
| `scene.parse_error` | Failed to parse scene |
| `scene.invalid` | Scene validation failed |
| `output.preset_not_found` | Output preset ID not recognized |
| `output.path_missing` | Output path not specified |
| `asset.missing` | Referenced asset not found |
| `asset.invalid` | Asset exists but cannot be decoded |
| `render.failed` | Render process failed |
| `cli.out_missing` | Missing output argument |
| `cli.invalid_flag` | Invalid CLI flag or value |

---

## Progress Reporting

### JobProgress

```json
{
  "job_id": "render_abc123",
  "status": "running",
  "frames_done": 420,
  "frames_total": 900,
  "fps": 31.4,
  "eta_seconds": 15.3,
  "current_stage": "rendering"
}
```

**Stages**: `initializing`, `loading_assets`, `rendering`, `encoding`, `finalizing`, `completed`, `failed`

---

## Presets API

### List Presets

**Request**:
```json
GET /presets
```

**Response**:
```json
{
  "ok": true,
  "data": {
    "output_presets": ["youtube_1080p", "youtube_shorts", "youtube_4k", "preview"],
    "transitions": ["fade", "slide_left", "zoom", "glitch", "wipe_left"],
    "backgrounds": ["cinematic_dark", "youtube_news_blue", "soft_gradient"]
  }
}
```

### Get Preset Info

**Request**:
```json
GET /presets/youtube_shorts
```

**Response**:
```json
{
  "ok": true,
  "data": {
    "id": "youtube_shorts",
    "codec": "h264",
    "pixel_fmt": "yuv420p",
    "container": "mp4",
    "width": 1080,
    "height": 1920,
    "crf": 18,
    "color_space": "bt709"
  }
}
```

---

## Quality Presets

| Quality | Samples | Motion Blur | Denoise | Use Case |
|---------|----------|-------------|---------|----------|
| `draft` | 8 | No | No | Quick preview, testing |
| `final` | 64 | Yes | No | Production output |
| `max` | 256 | Yes | Yes | Maximum quality |

---

## Job Management

### Job Status

```json
GET /jobs/:id
```

```json
{
  "ok": true,
  "data": {
    "job_id": "render_abc123",
    "status": "running",
    "created_at": "2026-05-06T10:30:00Z",
    "progress": { ... }
  }
}
```

### List Jobs

```json
GET /jobs
```

```json
{
  "ok": true,
  "data": {
    "jobs": [
      { "job_id": "render_abc123", "status": "running" },
      { "job_id": "render_def456", "status": "completed" }
    ]
  }
}
```

### Cancel Job

```json
DELETE /jobs/:id
```

---

## Health Check

```json
GET /health
```

```json
{
  "ok": true,
  "data": {
    "status": "healthy",
    "version": "1.0.0",
    "build": "dev",
    "capabilities": {
      "transitions": 10,
      "backgrounds": 8,
      "output_presets": 6
    }
  }
}
```

---

## CLI Equivalents

| HTTP API | CLI Command |
|----------|-------------|
| `POST /render` | `tachyon render scene.cpp --out video.mp4` |
| `POST /preview` | `tachyon preview scene.cpp` |
| `POST /thumbnail` | `tachyon thumb scene.cpp` |
| `POST /validate` | `tachyon validate --cpp scene.cpp` |
| `GET /presets` | `tachyon output-presets list` |
| `GET /jobs/:id` | (future: `tachyon jobs status :id`) |
| `GET /health` | `tachyon doctor` |

---

## Notes

- All paths in requests are relative to the working directory unless absolute
- Preset IDs are case-sensitive
- Frame ranges are inclusive: "0-300" means frames 0 through 300
- Time strings support formats: "3s", "00:00:03", "00:03"
- The `seed` field ensures deterministic rendering across runs
