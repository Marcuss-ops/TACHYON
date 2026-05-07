# Domain Pipeline Debt Audit

This document identifies architectural debt that prevents Tachyon's domains (Background, Text, 2D, 3D) from converging into a single, unified pipeline.

## Identified Debt Items

### 1. Layer Type Ownership Drift
**Status**: ✅ SOLVED — Centralized into `LayerType` enum. Builders and Compiler now prioritize the enum. Fallbacks to `type_string` are minimal and only for migration compatibility.

### 2. Background Rendering Special Path
**Status**: ✅ SOLVED — Backgrounds resolve to standard layers at index 0 during `SceneCompiler` and `EvaluatorComposition`. Redundant `clear(state.background_color)` removed from `composition_renderer.cpp`.

### 3. Text Domain Special-Case Drift
**Status**: 🔶 IN PROGRESS — Text authored as `TextSpec` resolves to `LayerType::Text`. Further audit required for font fallback isolation and text effect parity with other layer types.

### 4. Mandatory 3D Coupling
**Status**: ✅ SOLVED — `RenderContext` uses forward declarations for 3D types. Concrete instantiation is strictly guarded by `TACHYON_ENABLE_3D` and performed only in `frame_executor.cpp`.

### 5. Unformalized 2D/3D Bridge
**Status**: ✅ SOLVED — Formalized in `docs/2d-3d-bridge.md`. 3D blocks are treated as producer surfaces for the 2D consumer. `IRayTracer` interface enforces the contract.

### 6. Duplicate Registry/Resolver Logic
**Status**: 🔶 IN PROGRESS — `TransitionResolver` and `EffectResolver` created at `src/core/transition/transition_resolver.cpp` and `src/renderer2d/effects/effect_resolver.cpp`. Hardcoded `switch` statements being migrated to use centralized resolvers. `docs/00-project/feature-extension-rules.md` enforces strict guardrails on new visual features.
### 7. Direct `renderer3d` Header Leakage in 2D Pipeline
**Status**: ✅ SOLVED (2026-05-06) — `composition_renderer.cpp` no longer includes `renderer3d/core/ray_tracer.h`. The fallback instantiation has been moved to `frame_executor.cpp` where it belongs.

## Architecture Invariants (Enforced)

These rules are now code-enforced and must not be regressed:

| Rule | Enforcement |
|---|---|
| `renderer2d` does not include `renderer3d` headers | Remove on PR review |
| `IRayTracer` is the only 3D type visible in `renderer2d` | Forward-declared in `render_context.h` |
| Concrete `RayTracer` allocated only in `frame_executor.cpp` | Single instantiation point |
| All 3D code inside `#ifdef TACHYON_ENABLE_3D` | Enforced at both injection and usage sites |

## Remaining Work

1. Consolidate effect and transition resolution into centralized registries (`#6 above`).
2. Text domain: isolate font fallback logic from the composition evaluator.
3. Verify "No 3D" build configuration (`TACHYON_ENABLE_3D=OFF`) compiles cleanly with no `renderer3d` link errors.
4. Add CI check that `grep -r "renderer3d" src/renderer2d/ include/tachyon/renderer2d/` returns zero results.
