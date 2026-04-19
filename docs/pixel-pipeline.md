# Pixel Pipeline

## Purpose

This document defines how evaluated scene state becomes pixels.

## Initial scope

- 2D raster compositing path
- explicit render passes
- 3D path-traced output composition
- alpha and auxiliary buffer handling
- deterministic color management integration

## Design rules

1. The pixel pipeline must consume resolved scene state, not raw authoring structures.
2. Pass boundaries should remain explicit for caching and denoising.
3. The pipeline should support both crisp 2D work and offline 3D output.
