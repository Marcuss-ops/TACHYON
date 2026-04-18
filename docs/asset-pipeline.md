# Asset Pipeline

## Purpose

This document defines how external media enters, lives inside, and is prepared for rendering in TACHYON.

## Asset categories

The engine should treat at least the following as first-class asset categories:

- images
- image sequences
- video
- audio
- fonts

## Core responsibilities

The asset pipeline should define:

- decode policy
- lifetime and residency rules
- scaling policy
- cache policy
- time mapping for media assets
- error behavior for missing or invalid assets

## CPU-first requirement

Because TACHYON targets headless and low-cost environments, the asset pipeline must reduce unnecessary decode and transform work.
Lazy loading and explicit caching are core design requirements.

## Stable identity

Every asset should have a stable asset id.
Layers should reference assets through ids rather than duplicate raw source paths.

## Memory discipline

The asset pipeline should eventually define practical budgets for:

- decoded frame residency
- image cache reuse
- font cache residency
- pre-scaled intermediates

## Decode boundaries

Decoding should be separable from scene evaluation and from final compositing.
This allows caching, profiling, and deterministic failure handling.

## Failure behavior

The engine should define whether missing assets produce validation failure, render-job failure, or explicit placeholder behavior.
That decision must be consistent and testable.
