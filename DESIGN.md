# TACHYON Architectural Truths

This document defines the core hierarchy of truth within the Tachyon project.

## 1. Truth Hierarchy

1.  **C++ Builder API** = Primary source of truth. The engine is authoring-centric, and type-safe C++ builders are the canonical way to define scenes.
2.  **SceneSpec** = Internal serializable model. This is the bridge between the builder and the runtime evaluation.
3.  **JSON** = Compatibility, fixture, and debug import/export format. JSON is **not** the primary authoring API.
4.  **CLI** = Thin wrapper above the Runtime API. Its responsibility is discovery and execution, not core logic.

## 2. Core Mandates

- **Headless First**: No browser, no DOM, no JS/TS in the render core.
- **Deterministic**: Every pixel must be programmatically derived and reproducible.
- **CPU Saturation**: Architecture optimized for 100% core utilization via tiling and taskflow.
- **Zero Waste**: SIMD-first (AVX2/FMA), SOA memory structures, and zero-allocation hot paths.

## 3. Public API (Minimal)

The public interface should be accessed via `include/tachyon/tachyon.h`, which provides a curated entry point to the Builder and Runtime Facade.
