# Diagnostics and Profiling

## Purpose

This document defines how TACHYON should explain its own behavior.

A headless engine becomes hard to operate very quickly if it can render frames but cannot clearly report where time, memory, or failures are coming from.

## Core rule

Every major pipeline stage should be observable.

At minimum, contributors and operators should be able to distinguish:
- validation problems
- evaluation problems
- render problems
- encode or mux problems
- resource-pressure problems

## Observability layers

Recommended layers:

### Structured logs
Human-readable and machine-consumable event records.

### Metrics and counters
Useful for aggregate reporting across many jobs.

### Per-job summaries
Compact execution reports for one render job.

### Profiling traces
Detailed timing and stage breakdowns for debugging and optimization.

## Log event model

The engine should prefer structured logs over free-form text only.

Useful common fields:
- timestamp
- severity
- subsystem
- job id
- composition id where relevant
- frame index where relevant
- event code
- human-readable message

## Severity levels

Recommended first levels:
- debug
- info
- warn
- error
- fatal

## Subsystem taxonomy

Useful first subsystem labels:
- spec
- validation
- bindings
- evaluation
- timeline
- rendergraph
- composite
- renderer_2d
- renderer_3d
- cache
- output
- encode
- runtime

## Event categories

The engine should expose event categories such as:
- job_started
- job_finished
- validation_failed
- frame_started
- frame_finished
- cache_hit
- cache_miss
- queue_pressure
- encode_backpressure
- disk_space_low later if available
- resource_budget_exceeded

## Per-job summary

Each completed or failed job should be able to emit a summary containing:
- job id
- selected quality tier
- input scene spec version
- output profile
- total wall time
- major stage timings
- peak memory estimate if available
- cache hit and miss counts
- final status

## Frame-level profiling

For expensive jobs, the engine should be able to report per-frame breakdowns.

Useful first fields:
- frame index
- evaluation time
- rendergraph planning time
- 2D render time
- 3D render time
- compositing time
- output handoff time
- encode wait time if measurable

## Queue diagnostics

The runtime should expose queue behavior clearly.

Recommended signals:
- current length
- maximum configured length
- high-water mark
- producer blocked duration
- consumer starvation duration where useful

This is especially important for render to encode handoff.

## Cache diagnostics

The cache layer should expose:
- cache unit type
- hit or miss result
- eviction reason where relevant
- bytes retained or evicted where measurable
- cache pressure events

## Failure diagnostics

Failures should be explicit and classified.

Recommended error classes:
- invalid_scene_spec
- invalid_render_job
- missing_asset
- evaluation_error
- render_backend_error
- encode_backend_error
- disk_full
- memory_exhausted
- unsupported_output_profile
- compatibility_mismatch later where relevant

## Profiling modes

Recommended first modes:

### Normal
Low-overhead logs and summary only.

### Profiled
Adds per-stage timing and richer counters.

### Trace
Highest-detail execution tracing for engine debugging.

## Output formats

The engine should support at least:
- human-readable console output
- structured JSON summary files or streams later where useful

## Determinism note

Observability should not alter render semantics.
Profiling may slow the job down, but it must not change the produced output.

## First implementation recommendation

The first implementation should support:
- structured severity-based logs
- subsystem labels
- per-job summary
- frame timing totals
- cache hit or miss visibility
- encode backpressure visibility
- classified fatal failure reasons

## Guiding principle

TACHYON should not only render deterministically.
It should also explain deterministically where time and failures are going.
That is what makes a headless engine operable in real automation workflows.
