---
Status: Canonical
Last reviewed: 2026-05-06
Owner: Core Team
---

# Tachyon Test Matrix Mapping

To maintain a manageable test suite, all tests must be routed to the appropriate executable target based on their domain.

## Target Mapping

| Domain / Directory | Target Executable | Purpose |
|--------------------|-------------------|---------|
| `core/*` (math, spec, logic) | `TachyonTests` | Low-level engine core and primitives. |
| `scene/*` (eval, inspector) | `TachyonSceneTests` | Scene graph and evaluation logic. |
| `runtime/execution/*` | `TachyonRuntimeTests` | Frame execution, tiling, and scheduling. |
| `renderer2d/*` | `TachyonRenderTests` | 2D rasterization and composition. |
| `content/presets/*` | `TachyonContentTests` | Builder contracts and preset registries. |
| `integration/*` | `TachyonCliTests` | End-to-end CLI and vertical slices. |

## Rules for Adding Tests

1. **Prefer existing targets**: Do not create a new executable target unless the dependency set is significantly different.
2. **Library Linking**: Ensure the target links only against the necessary engine sub-libraries to keep link times low.
3. **Naming**: Use the suffix `_tests.cpp` for all test source files.
4. **Main**: Each target should have a dedicated `test_main_<target>.cpp` to manage its own test runner initialization.

## CI Enforcement

The `scripts/agent-validate.ps1` (and its Linux equivalent) uses these targets to scope validation. Incorrectly routed tests may be skipped during targeted validation runs.
