# Voice Over and Subtitles

## Purpose

This document defines the timing and burn-in side of spoken and text-driven media workflows.

## Initial scope

- subtitle timing
- subtitle burn-in
- forced alignment hooks
- voice-over timing integration points

## Design rules

1. Timing data should live in the evaluation layer, not inside the renderer.
2. Subtitle workflows must remain deterministic and cacheable.
3. The system should support future per-word subtitle animation.
