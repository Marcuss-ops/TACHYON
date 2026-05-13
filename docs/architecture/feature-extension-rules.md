# Tachyon Feature Extension Rules

To prevent technical debt and maintain architectural integrity, all new features and extensions must follow these mandatory rules.

## 1. Single Source of Truth (Canonical Model)
- Every domain must have a single canonical specification (e.g., `TextAnimatorSpec`, `EffectBuiltinSpec`, `ModifierSpatialSpec`).
- **DO NOT** add ad-hoc fields to runtime structures (like `TextAnimationOptions` or `RenderSession`).
- All state must derive from the declarative spec and its associated sampling logic.

## 2. Explicit Dependency Injection
- **DO NOT** use `instance()` (Singleton) patterns for registries or factories in core evaluation/rendering paths.
- Registries must be passed explicitly through `RenderContext`, `RenderSession`, or as function parameters.
- This ensures testability and prevents hidden global state.

## 3. No Runtime Fallbacks
- The rendering loop must only understand the canonical model.
- **DO NOT** implement "legacy support" logic inside the hot paths of `layout.cpp` or `renderer2d/Spatial` modules.
- If backward compatibility is required, implement it as a **Shim/Adapter** that translates old data into the new canonical spec *before* it reaches the runtime.

## 4. Single Execution Pipeline
- Avoid creating parallel pipelines for the same feature.
- Extend the existing registry and sampler systems instead of duplicating logic.
- If a feature is "too different," consider if it belongs in a new domain or if the existing domain should be refactored first.

## 5. Automated Guardrails
- Banned symbols (e.g., `instance()`, `per_glyph_offset_x`) are monitored via grep-based checks in CI.
- PRs that introduce new singletons or legacy runtime fields will be rejected.

---
*Status: Mandatory*
*Last Reviewed: 2026-05-07*
