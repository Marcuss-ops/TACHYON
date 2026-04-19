# Multi-Format Output

## Purpose

This document defines output targets beyond a single default render preset.

## Initial scope

- standard video deliverables
- square and vertical variants
- thumbnail-friendly frame outputs
- batch-oriented render job overrides

## Design rules

1. Output format selection should stay separate from scene definition.
2. The same scene should be reusable for multiple deliverable formats.
3. Output behavior must remain deterministic for the same inputs.
