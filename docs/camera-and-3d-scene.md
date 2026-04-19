# Camera and 3D Scene

## Purpose

This document covers the scene-side 3D camera and spatial node model.

## Initial scope

- perspective camera
- orthographic camera later
- focal length
- aperture and depth of field
- focus distance
- camera motion blur
- 3D transform hierarchy

## Design rules

1. Camera state must be animatable like any other property.
2. The camera should feed the render graph, not directly own pixel output.
3. Scene evaluation must remain deterministic across frames.
