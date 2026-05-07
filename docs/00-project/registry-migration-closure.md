---
Status: Canonical
Last reviewed: 2026-05-06
Owner: Core Team
Supersedes: N/A
---

# Registry Migration Closure

This document tracks the final steps required to close the transition from legacy singleton registries to the canonical Builder-based architecture.

## Goal
Eliminate all residual singleton registries and ensure that all domain extensions follow the [Presets Contract](../10-architecture/presets-contract.md).

## Checklist

### 1. Legacy API Removal
- [x] Remove `TransitionRegistry::instance()` and replace with local dispatch in `build_transition()`.
- [x] Remove `TransitionCatalog` singleton.
- [x] Remove `BackgroundCatalog` singleton.
- [x] Clean up `include/tachyon/transition_registry.h`.

### 2. Alias & Legacy Cleanup
- [x] Remove `TransitionSpec::cpu_fn_name` if `TransitionFn` is now directly assigned.
- [ ] Remove legacy string-based type fields in `LayerSpec` that were kept for compatibility.
- [x] Remove `TachyonTransitionHandle` if the C API has been migrated to `LayerSpec` based transitions.

### 3. Registry Contract Tests
- [ ] Ensure `tests/unit/presets/contract_tests.cpp` covers all approved domains.
- [ ] Verify that no new domain introduces a `register_*` pattern.

### 4. Dispatcher Architecture
- [ ] Verify "No double dispatcher": there should be only one entry point per domain (e.g., `build_text_layer`).
- [ ] Internal registries used for implementation details (e.g., within `src/`) must not be exposed in `include/`.

## Status
- **Phase 1 (Enum Migration)**: ✅ Completed.
- **Phase 2 (Singleton Removal)**: 🏗️ In Progress.
- **Phase 3 (Final Cleanup)**: ⏳ Scheduled.
