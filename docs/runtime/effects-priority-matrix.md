# Effects Priority Matrix

## Purpose

This document defines the first effect set that should be implemented for TACHYON based on practical value for automated motion graphics and YouTube-ready rendering.

## Principle

The first effect set should maximize useful visual range per unit of implementation complexity.

The engine does not need one hundred effects to become real.
It needs a small set that combines well and covers most production needs.

## Tier 1 — Must-have first effects

These effects provide the highest immediate value and should be considered first-wave builtins.

- Gaussian Blur
- Glow
- Drop Shadow
- Levels
- Curves
- Hue Saturation
- Fill
- Tint
- Gradient Ramp
- Feather
- Stroke

## Tier 2 — High-value next effects

These effects materially expand the look range for video templates and stylized motion graphics.

- Fractal Noise
- Displacement Map
- Vignette
- Film Grain
- Chromatic Aberration
- LUT Apply

## Tier 3 — Valuable but later

These effects are useful, but should wait until pass contracts and performance behavior are stable.

- Camera Lens Blur
- Directional Blur
- Radial Blur
- Turbulent Displace
- Edge Detect
- Matte Choker or shrink-grow variants
- Sharpen or unsharp mask

## Effect grouping

### Color effects

- Levels
- Curves
- Hue Saturation
- Fill
- Tint
- LUT Apply

### Light and glow effects

- Glow
- Vignette
- Film Grain
- Chromatic Aberration

### Spatial effects

- Gaussian Blur
- Feather
- Stroke
- Displacement Map

### Generative support effects

- Gradient Ramp
- Fractal Noise

## First implementation rules

The first implementation of each effect should define:

- deterministic parameter semantics
- parameter typing
- region of interest behavior
- premultiplied alpha behavior
- color-space expectations
- precision expectations
- whether depth or auxiliary inputs are required

## Recommended implementation order

1. Gaussian Blur
2. Fill and Tint
3. Levels and Curves
4. Drop Shadow
5. Glow
6. Gradient Ramp
7. Stroke
8. Feather
9. Hue Saturation
10. LUT Apply
11. Fractal Noise
12. Displacement Map
13. Vignette
14. Film Grain
15. Chromatic Aberration

## Notes

Camera Lens Blur should not be rushed if it requires pass contracts or depth semantics that are not yet stable.

The first effect wave should bias toward effects that are easy to explain, easy to cache, and visibly useful in automated video rendering.
