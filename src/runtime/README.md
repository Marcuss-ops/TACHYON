# Runtime Module

Runtime execution engine for rendering, playback, caching, and serialization.

## Directory Structure

```
src/runtime/
├── core/              # Core runtime data structures
│   ├── data/         # Compiled scenes, graphs
│   ├── graph/        # Render graph execution
│   └── serialization/ # TBF binary format
├── execution/         # Render execution pipeline
│   ├── frame_executor/ # Per-frame evaluation
│   ├── render_job/   # Job parsing and validation
│   ├── batch_runner/ # Batch rendering
│   └── render_plan/  # Render planning
├── cache/             # Caching system
│   ├── frame_cache.cpp
│   └── disk_cache.cpp
├── resource/          # Resource management
└── playback/          # Playback engine
```

## Key Components

### Execution Pipeline (`execution/`)
- `render_session.cpp`: Main render session orchestrator
- `frame_executor.cpp`: Per-frame evaluation and rendering
- `render_plan_build.cpp`: Builds render plans from scenes
- `render_plan_hash.cpp`: Hash-based plan caching

### Serialization (`core/serialization/`)
- `tbf_codec.cpp`: Binary format encoder/decoder with version migration

### Caching (`cache/`)
- `frame_cache.cpp`: In-memory frame cache
- `disk_cache.cpp`: Persistent disk caching
- `cache_key_builder.cpp`: Cache key generation

## TBF Format
Tachyon Binary Format (TBF) is the compiled scene format with version migration support (v1-v5).
