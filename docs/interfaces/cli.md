# CLI

## Purpose

This document defines the headless command-line surface for TACHYON.

## Core commands

- render a scene spec
- validate a scene spec
- inspect a render job
- list or resolve assets
- print engine/version information

## Minimum shape

```text
tachyon render --scene scene.json --job job.json --out out.mp4
tachyon validate --scene scene.json
tachyon info
```

The initial implementation supports `version`, `validate`, and a contract-checking `render` command that validates scene and job inputs before any pixel backend exists.

## Required behavior

- exit codes must distinguish validation failure, runtime failure, and success
- logging must be machine-readable by default
- batch rendering must not require UI assumptions
- deterministic jobs should be reproducible from the same CLI inputs

## Design rules

1. CLI flags should map directly to engine concepts.
2. Output paths and formats must be explicit.
3. The CLI should be usable in CI and server environments without wrappers.
