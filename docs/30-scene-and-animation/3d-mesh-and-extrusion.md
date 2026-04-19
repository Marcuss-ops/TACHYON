# 3D Mesh and Extrusion

## Purpose

This document defines the narrow first 3D geometry scope for TACHYON.

## Initial scope

- mesh layers
- glTF mesh import
- extruded text
- extruded shapes
- compact geometry conversion for CPU path tracing

## Design rules

1. Geometry must be deterministic and cacheable.
2. Extrusion should stay simple enough to integrate with Embree-backed rendering.
3. The first release should prioritize title cards, logos, and product-style shots over broad modeling features.
