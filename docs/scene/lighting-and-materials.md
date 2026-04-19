# Lighting and Materials

## Purpose

This document defines the first physically based lighting and shading model for the 3D path.

## Initial scope

- directional lights
- point lights
- spot lights
- area lights
- environment lighting via HDRI
- compact PBR materials
- shadowing, reflections, refractions, and GI through the path tracer

## Design rules

1. Lighting must remain deterministic.
2. Materials should be explicit and small before any larger node system exists.
3. The first goal is credible offline output, not exhaustive shading flexibility.
