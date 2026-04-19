# Deterministic Caching

## Purpose

Deterministic caching is a core performance system for a programmatic motion engine.
It allows expensive work to be reused safely when scene inputs have not changed.

## Cache philosophy

A cache entry should be reusable only when its full dependency set is equivalent.
At a high level, cache identity should eventually derive from a stable key built from:

- relevant scene state
- time or frame domain
- asset identities
- effect parameters
- template overrides
- engine version or compatibility scope where necessary

## Architectural role

Caching belongs to render planning and incremental execution.
It should apply to more than just final frames.
Potential cacheable units include:

- precompositions
- effect outputs
- intermediate render targets
- evaluated property subgraphs

## Planned structure

```text
src/rendergraph/
├── cache_key.cpp
└── incremental_graph.cpp

src/profile/
└── memoization.cpp
```

## Design rules

1. Cache hits must never compromise determinism.
2. Cache invalidation must follow explicit dependency rules.
3. Cache units should be aligned with the render graph and scene evaluation model.
4. The engine should prefer correctness first, then reuse.
