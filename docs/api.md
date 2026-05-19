# TACHYON Public API Contract

TACHYON is a high-performance, deterministic, headless 2D motion graphics engine. This document defines the canonical entry points and public builder APIs for C++ developers.

---

## 1. Core Principles

- **C++ First**: The primary and canonical way to author scenes is via the C++ fluent API.
- **Type-safe Builders**: Bypasses the ambiguity and error-proneness of direct JSON specifications.
- **Stateless Runtime**: The rendering core maintains no global state; context is orchestrated via the `EngineRegistry`.
- **Headless**: Zero GUI or browser engine requirements in the render loop.

---

## 2. Programmatic Scene Authoring (C++)

TACHYON provides a native, highly expressive, Remotion-style fluent API for assembling composition timelines, audio tracks, and layers programmatically.

```cpp
#include "tachyon/scene/builder.h"
#include "tachyon/presets/shape/fluent.h"
#include "tachyon/presets/background/fluent.h"
#include "tachyon/presets/audio/fluent.h"

// Entrypoint definition for C++ builder plugins
extern "C" void build_scene(tachyon::SceneSpec& out) {
    using namespace tachyon;
    using namespace tachyon::presets;
    
    out = SceneBuilder()
        .project("promo_001", "Product Reveal Video")
        .composition("main", [](CompositionBuilder& c) {
            c.size(1920, 1080)
             .fps(30)
             .duration(10.0);
             
            // 1. Procedural Aura Background Layer
            c.layer(background::aura()
                .width(1920)
                .height(1080)
                .duration(10.0)
                .seed(42)
                .palette(background::palettes::neon_night())
                .grain(0.12)
                .build());
             
            // 2. Reactive Audio Track
            c.audio(audio::track("background_music")
                .source("assets/music.wav")
                .volume(0.85)
                .fade_in(0.2)
                .normalize_lufs(-14.0)
                .build());
             
            // 3. Dynamic Center Shape Overlay with Animations
            c.layer(shape::rectangle()
                .width(400)
                .height(200)
                .color({255, 255, 255, 255})
                .center()
                .animate(shape::fade_in().duration(0.5).build())
                .build());
        })
        .build();
}
```

---

## 3. Canonical Compilation & Execution

After building the declarative scene specification (`SceneSpec`), rendering and encoding jobs are dispatched through the stateless runtime:

```cpp
#include "tachyon/tachyon.h"
#include "tachyon/runtime/runtime_facade.h"

void execute_render_pipeline() {
    // 1. Initialize the global Engine Registry (caches and configurations)
    auto registry = tachyon::create_default_context();

    // 2. Generate scene specification via builder
    tachyon::SceneSpec spec;
    build_scene(spec);

    // 3. Compile and optimize the scene timeline graph
    auto compiled = tachyon::RuntimeFacade::instance().compile_scene(spec);

    if (compiled.ok()) {
        // 4. Render and encode composition directly to disk
        tachyon::RuntimeFacade::instance().export_video(
            *compiled.value, 
            "output/rendered_promo.mp4"
        );
    }
}
```

---

## 4. Public Header Boundary

Only headers located in the public `include/tachyon/` directory are considered stable APIs. Specifically:

| Header | Description |
|---|---|
| `include/tachyon/tachyon.h` | Global initialization and umbrella header |
| `include/tachyon/scene/builder.h` | Declarative timeline, track, and layer builders |
| `include/tachyon/runtime/runtime_facade.h` | Scene compilation, validation, and rendering orchestration |
| `include/tachyon/core/types/diagnostics.h` | Diagnostics and diagnostics reporting mechanisms |

> [!WARNING]
> Internal helper headers (located under `include/tachyon/detail/` or compiled directly inside the `src/` directory) do not provide stability guarantees and are subject to change without warning.
