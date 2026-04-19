# Motion Blur

## Purpose

This document defines motion blur as an explicit render feature.

## Initial scope

- object motion blur
- camera motion blur
- shutter interval definition
- deterministic sampling across exposure

## Design rules

1. Motion blur must be driven by scene time and evaluation state.
2. It should not be an accidental post-process approximation.
3. The feature must be compatible with both 2D compositing and CPU path tracing.
