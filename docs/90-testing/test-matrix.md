# Test Matrix

This document maps source areas to the primary CTest targets used in Tachyon.

## Routing Rules

| Source area | Primary test target | Labels |
|---|---|---|
| `src/core/*` | `TachyonTests` | `core` |
| `src/core/scene/*` | `TachyonSceneTests` | `core`, `scene` |
| `src/runtime/cache/*` | `TachyonRuntimeTests` | `runtime`, `deterministic` |
| `src/runtime/execution/*` | `TachyonRuntimeTests` | `runtime`, `deterministic` |
| `src/runtime/profiling/*` | `TachyonRenderProfilerTests` | `runtime`, `deterministic` |
| `src/runtime/execution/session/*` | `TachyonNativeRenderTests` | `runtime`, `render` |
| `src/renderer2d/*` | `TachyonRenderTests` or `TachyonRenderPipelineTests` | `render`, `renderer2d` |
| `src/presets/*` | `TachyonContentTests` | `content` |
| `src/text/*` | `TachyonContentTests` | `content` |
| `src/audio/*` | `TachyonContentTests` | `content` |

## Notes

- Add new runtime execution code to `TachyonRuntimeTests` unless it is purely render-session orchestration.
- Add new renderer2d rendering behavior to `TachyonRenderTests` or `TachyonRenderPipelineTests`.
- Add new scene authoring or validation behavior to `TachyonSceneTests` first.
- Keep smoke coverage fast; smoke should be a subset of core behavior, not a replacement.
