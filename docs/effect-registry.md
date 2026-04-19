# Effect Registry

## Purpose

This document defines the minimal built-in effect catalog for TACHYON.

## Registry responsibilities

- assign stable effect ids
- define parameter names and types
- document units and defaults
- declare required inputs and auxiliary buffers
- declare ROI behavior
- declare whether an effect needs depth or time data

## Initial built-in effects

- gaussian blur
- glow
- drop shadow
- levels or curves
- hue saturation
- fill
- tint
- fractal noise
- displacement map
- gradient ramp
- feather
- stroke
- camera lens blur
- chromatic aberration
- vignette
- film grain
- LUT application

## Design rules

1. Effects must be deterministic.
2. Effects must not depend on hidden global state.
3. Every effect should be cacheable from explicit inputs.
4. The registry should support future extension without breaking existing ids.
