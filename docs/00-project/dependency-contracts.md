# Dependency Contracts

> Status: Canonical policy
> Last reviewed: 2026-05-06

## Allowed Direction

- `Core` stays dependency-light and should not directly depend on renderer or output internals.
- `Renderer2D` should not depend on `Renderer3D` except through narrow bridge types.
- `Media` should resolve assets and decode content, not own renderer policy.
- `Runtime` orchestrates the session and execution plan, but should not become the home for subsystem-specific policy.

## Current Mitigations

- Dependency groups are split into `cmake/deps/*.cmake`.
- Shared options are centralized in `cmake/options/TachyonOptions.cmake`.
- Build dependencies are pinned in `cmake/deps/DependencyVersions.cmake`.

## Follow-Up

- Add an automated target-dependency policy check.
- Keep new cross-layer links narrow and documented.
- Prefer adapters or bridge types over broad public headers.
