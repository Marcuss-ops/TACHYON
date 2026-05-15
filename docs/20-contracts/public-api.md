# Tachyon Public API Contract

TACHYON is a high-performance, deterministic, headless motion graphics engine. This document defines the canonical entry points for C++ developers.

## 1. Core Principles

- **C++ First**: The primary way to use Tachyon is via the C++ API.
- **Headless**: No UI dependencies in the core.
- **Deterministic**: Given the same `SceneSpec` and `EngineRegistry`, the output is bit-identical.
- **Stateless**: The engine core does not hold global state; context is passed via registries.

## 2. Canonical Pipeline

The engine follows a strict linear pipeline:

1.  **Authoring Intent**: Use `SceneBuilder` to define layers, timing, and properties.
2.  **Scene Specification**: `SceneBuilder::build()` generates a `SceneSpec`.
3.  **Validation**: Use `RuntimeFacade::validate_scene(spec)` to ensure the spec is valid.
4.  **Compilation**: Use `RuntimeFacade::compile_scene(spec)` to generate a `CompiledScene`.
5.  **Execution**: Use `RuntimeFacade::render_frame()` or `RuntimeFacade::export_video()`.

```cpp
#include <tachyon/tachyon.h>

void example() {
    // 1. Setup Registry
    auto registry = tachyon::create_default_context();

    // 2. Build Scene
    auto scene_spec = tachyon::scene::SceneBuilder()
        .add_layer(...)
        .build();

    // 3. Compile
    auto compiled = tachyon::RuntimeFacade::instance().compile_scene(scene_spec);

    // 4. Render
    if (compiled.ok()) {
        auto frame = tachyon::RuntimeFacade::instance().render_frame(*compiled.value, 0);
    }
}
```

## 3. Public Header Boundary

Only headers in `include/tachyon/` are considered stable public API. Specifically:

- `tachyon/tachyon.h`: Main umbrella header.
- `tachyon/scene/builder.h`: Scene construction.
- `tachyon/runtime/runtime_facade.h`: Engine execution.
- `tachyon/core/spec/schema/objects/scene_spec.h`: Data model.

## 4. Stability Guarantees

- **Stable**: The canonical pipeline and `SceneBuilder` methods.
- **Experimental**: HTTP/JSON interfaces, `TachyonServer`, and advanced SIMD optimizations.
- **Internal**: Anything in `src/` or `include/tachyon/detail/`.
