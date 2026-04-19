# Determinism

## Purpose

Determinism is a foundational requirement for TACHYON.
It is not only about reproducibility. It is also what makes caching, testing, parallel execution, and large-scale automated rendering reliable.

## Core requirement

Given the same:

- scene spec
- template overrides
- data inputs
- assets
- engine version or compatibility target
- seed values

The evaluated result at frame t and pixel position x,y should be reproducible.

## Why it matters

Determinism enables:

- visual regression testing
- stable cache reuse
- distributed rendering confidence
- consistent output across environments
- reliable template rendering at scale

## Sources of non-determinism to avoid

- hidden mutable global state
- unseeded randomness
- order-dependent floating behavior where avoidable
- implicit dependence on evaluation scheduling
- time-of-day or environment-dependent execution
- uncontrolled parallel write behavior

## Determinism contract

- seed policy must be part of the render job or scene version
- sample ordering must be stable for the same job and engine version
- floating-point precision assumptions must be documented
- cache keys must include all visible inputs
- any compatibility-breaking change must be versioned intentionally

## Architectural implications

Determinism affects:

- expression evaluation
- data binding
- animation evaluation
- render graph execution
- caching
- asset sampling
- effect processing

## Design rules

1. Every subsystem should make its inputs explicit.
2. Random behavior must be seeded and reproducible.
3. Parallel execution must not alter results.
4. Cache keys must be based on explicit inputs, not incidental process state.
5. Engine behavior that changes across versions must be versioned intentionally.

## Guiding principle

Determinism is the condition that allows TACHYON to behave like infrastructure rather than like a best-effort creative tool.
