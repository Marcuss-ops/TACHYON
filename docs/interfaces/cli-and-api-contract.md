# CLI and API Contract

## Purpose

This document defines the first external control surface for TACHYON.

A programmable motion engine needs a clean interface boundary.
Without it, orchestration code, automation tools, and operators will end up depending on unstable internal implementation details.

## Core rule

The external interface should express engine intent, not internal hacks.

That means:
- stable commands or API calls
- structured inputs
- structured outputs
- clear exit and error behavior
- no dependence on implicit environment quirks

## Interface shapes

The first implementation should support at least:
- CLI entrypoints
- a library or service-facing API later if needed

The CLI should be treated as the first-class public contract even if deeper embedding is added later.

## Recommended first CLI commands

### `tachyon validate`
Validate a scene spec and optionally a render job without rendering.

### `tachyon inspect`
Inspect scene metadata, compositions, published properties, or asset dependencies.

### `tachyon render`
Render a job and produce final outputs.

### `tachyon profile`
Render with profiling or rich diagnostics enabled.

## Input model

The CLI should prefer explicit file-based inputs or explicit structured payloads.

Recommended first inputs:
- `--scene <path>`
- `--job <path>`
- `--output <path>` where appropriate
- `--quality-tier <name>` override where allowed
- `--profile <name>` output profile override where allowed

## Output model

CLI outputs should include:
- human-readable status messages
- stable exit codes
- optional structured summary output later if requested

## Exit code policy

The first CLI contract should define stable categories such as:
- `0` success
- non-zero for failure

Recommended first failure categories:
- validation failure
- missing input or asset
- render failure
- encode failure
- resource exhaustion

The exact numeric mapping can be documented once implementation begins, but the categories should be fixed.

## Structured summary

The interface should leave room for a machine-readable summary artifact or stdout mode containing:
- job id
- output paths
- final status
- selected quality tier
- output profile
- timing summary
- cache statistics later where available

## API contract direction

If TACHYON later exposes a library or service-facing API, it should mirror the CLI concepts rather than inventing a second mental model.

Recommended first API concepts:
- validate_scene
- inspect_scene
- render_job
- profile_job

## Error contract

Errors should be classified and structured.

Useful first fields:
- code
- message
- subsystem
- file or asset path where relevant
- frame or composition context where relevant
- recoverability hint later if useful

## Compatibility rule

The CLI and API should evolve deliberately.
Breaking changes to flags, field names, or output structures should be treated as interface-significant changes and versioned or documented clearly.

## First implementation recommendation

The first stable CLI should support:
- validate
- inspect
- render
- profile
- scene path input
- render job path input
- explicit output override where relevant
- quality tier override
- output profile override

## Guiding principle

TACHYON should feel programmable from the outside, not only elegant on the inside.
A strong CLI and API contract is part of the product, not a wrapper added at the end.
