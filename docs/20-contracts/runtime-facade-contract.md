# Runtime Facade Contract

> **Status**: Stable / Canonical
> **Scope**: Defines the internal C++ contract for the Tachyon Runtime Facade.

---

## Guiding Principles

1. **Single Runtime Facade** - All interfaces (CLI, tests, future APIs) must use the same internal C++ facade (`RuntimeFacade`).
2. **Deterministic** - Same input produces same output (given same seed and assets).
3. **Structured Diagnostics** - All operations return detailed diagnostics (warnings, errors, performance metrics).
4. **Stateless Operations** - Each render/validate/preview call is independent.
5. **Asset Resolution** - Assets are resolved at render time via the `AssetResolutionTable`.

---

## Facade API (C++)

### Validation

```cpp
ValidationResult validate_scene(const SceneSpec& scene);
```
- Performs deep schema validation.
- Checks for temporal overlaps (if applicable).
- Verifies property bounds.

### Rendering

```cpp
RenderSessionResult render(
    const SceneSpec& scene,
    const RenderJob& job,
    const TransitionRegistry& transitions,
    const presets::TextRegistry& text_registry,
    const NativeRenderOptions& options);
```
- Full pipeline execution.
- Returns frames (if in-memory) or writes to disk (if file-based).
- Includes comprehensive `FrameDiagnostics`.

### Inspection

```cpp
InspectionReport inspect_scene(
    const SceneSpec& scene,
    const TransitionRegistry& transitions,
    const InspectionOptions& options);
```
- Static analysis of the scene.
- Identifies "hotspots" or potential rendering bottlenecks.

---

## Quality Presets (C++)

| Quality | Samples | Motion Blur | Denoise | Use Case |
|---------|----------|-------------|---------|----------|
| `draft` | 8 | No | No | Quick preview, testing |
| `final` | 64 | Yes | No | Production output |
| `max` | 256 | Yes | Yes | Maximum quality |

---

## Error Handling

The facade uses `core::MediaResult<T>` and `core::MediaError` to propagate failures. Errors include:
- `Stage`: Where the error occurred (e.g., `scene_compile`, `render_loop`).
- `Code`: Machine-readable error code.
- `Message`: Human-readable description.
