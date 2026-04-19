# Performance Tiers

## Purpose

This document defines quality and performance modes for hardware with different CPU and memory budgets.

## Tiers

### Draft

- low sample counts
- proxy assets
- reduced passes
- minimal motion blur
- fastest turnaround

### High

- balanced sample counts
- full compositing fidelity
- denoise when useful
- production default for most work

### Cinematic

- highest sample counts
- full 3D quality
- motion blur and DOF at full fidelity
- overnight-friendly render behavior

## Design rules

1. Tiers must be explicit and reproducible.
2. Lower tiers should degrade quality in a predictable way.
3. The same scene should be able to render under multiple tier policies.
