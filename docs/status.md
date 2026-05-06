# Project Status

> Status: Canonical
> Last reviewed: 2026-05-06
> Owner: Tachyon maintainers
> Supersedes: scattered status notes in planning docs, audit notes, and agent-facing checklists

## Stable

- Core math, property, expression, and scene-spec authoring primitives.
- The `TachyonCore` build target and the default developer build loop.
- The smoke-test path used by local quick checks and CI.

## Experimental

- 3D rendering and temporal features that are still gated by feature flags.
- Tracker-based features.
- Audio muxing and other runtime output paths that depend on optional integration points.
- Catalog tooling and editor-adjacent utilities.

## Deprecated Or In Transition

- JSON-first scene and render-job authoring as a primary workflow.
- Disabled tests tracked in `tests/disabled/README.md` until each one is either fixed or explicitly quarantined.

## Notes

- Prefer builder-based authoring for new work.
- Keep this file aligned with `tests/disabled/README.md`, `README.md`, and the root CMake options.
