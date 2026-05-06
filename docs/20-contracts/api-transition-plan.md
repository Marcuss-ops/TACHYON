---
Status: Canonical
Last reviewed: 2026-05-06
Owner: Core Team
Supersedes: README legacy notes
---

# Public API Transition Plan: JSON-first to C++-first

## Context
Tachyon is transitioning from a workflow where JSON scene files were the primary source of truth to a **C++ Builder API** centric architecture.

## Why C++-first?
1. **Type Safety**: Prevent malformed scenes at compile time.
2. **Determinism**: Enforce exact property types and default values.
3. **Performance**: Avoid heavy JSON parsing in the hot path.
4. **Developer Experience**: IDE autocompletion and better documentation discovery.

## Transition Strategy

### Phase 1: C++ Builder Parity (Completed)
- Implement `SceneBuilder`, `LayerBuilder`, and domain-specific builders.
- Ensure all features available in JSON are expressible in C++.

### Phase 2: JSON as "Serialization only" (Current)
- JSON import is maintained for backward compatibility and fixtures.
- JSON export is maintained for debugging and "frozen" scene delivery.
- **Source of truth**: The C++ code that calls the Builder.

### Phase 3: Schema Versioning & Deprecation
- Introduce versioning in `SceneSpec`.
- Deprecate ad-hoc JSON fields in favor of structured blocks.
- **JSON Import Support Level**: Maintenance only. No new features will be added to the JSON parser before they are in the C++ Builder.
- **JSON Export Support Level**: Full. Always support exporting a C++ built scene to JSON for inspection.

## Implementation Checklist

- [ ] Define `SceneSpec` versioning strategy.
- [ ] Implement `Builder` -> `JSON` round-trip compatibility tests.
- [ ] Update documentation to show C++ examples by default.
- [ ] Mark legacy JSON-only fields as `[[deprecated]]` in internal specs if possible.
