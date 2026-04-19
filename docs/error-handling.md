# Error Handling

## Purpose

This document defines how TACHYON should fail, report problems, and recover where possible.

## Error classes

- validation errors
- missing asset errors
- unsupported feature errors
- decode and encode errors
- render runtime errors
- cache and memory pressure errors

## Behavior goals

- fail fast on malformed specs
- report the exact path or object that failed
- distinguish recoverable placeholder behavior from hard failure
- keep diagnostics deterministic and testable

## Design rules

1. Errors should be structured.
2. The engine should never silently ignore unsupported authored intent.
3. Diagnostics should be useful in CI and batch rendering.
