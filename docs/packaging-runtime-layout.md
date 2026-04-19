# Packaging and Runtime Layout

## Purpose

This document defines project, cache, and output layout conventions for headless rendering.

## Layout goals

- keep project files portable
- keep cache locations explicit
- keep outputs separate from sources
- support CI and server execution

## Suggested structure

- `project/` for authored scene files
- `assets/` for source media
- `cache/` for decoded or intermediate data
- `out/` for rendered output
- `logs/` for diagnostics

## Design rules

1. Runtime layout should be explicit, not implicit.
2. Cache paths should be deterministic and configurable.
3. Output layout should be obvious for batch jobs and automation.
