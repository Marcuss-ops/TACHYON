# Decode and Encode

## Purpose

This document defines media I/O around the render pipeline.

## Initial scope

- native image decode
- native video decode
- audio decode for reactivity and timing
- video encode output
- image sequence support later if needed

## Design rules

1. Decode and encode should stay outside the render core.
2. Media I/O must be explicit and reproducible.
3. The engine should be able to render without depending on browser runtimes or external playback stacks.
