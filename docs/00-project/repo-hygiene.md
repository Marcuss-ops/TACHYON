# Repository Hygiene

> Status: Canonical
> Last reviewed: 2026-05-06
> Owner: Tachyon maintainers

## Root Directory Policy

- Keep the repository root limited to canonical project files: build scripts, top-level docs, and source tree entrypoints.
- Do not add ad-hoc debug scenes, scratch scripts, agent reports, or render-job dumps to the root.
- Temporary work should live under ignored scratch locations or under `tests/fixtures/`, `tests/golden/`, or `examples/` when it is meant to become a reusable fixture.

## Agent Artifacts

- Agent scratch content belongs in `.claude/`, `.gemini/`, or another ignored local workspace path.
- Do not commit local transcripts, debug exporters, or one-off helper scripts unless they are promoted into a documented tool.

## Review Rule

- If a file exists only to diagnose one bug, it should either move to a fixture directory or be deleted after the fix lands.
