# MVP v1

## Purpose

This document defines the first implementation slice that should make TACHYON real without exploding scope.

## MVP goal

Deliver a small but serious vertical slice of a deterministic headless motion graphics engine.

## In scope

- scene spec v0
- one renderable composition
- solid, image, text, shape, precomp, null, and camera layers
- 2D transforms and a narrow 2.5D camera model
- opacity and basic blending
- masks and a basic track matte path
- a minimal effect set
- template overrides
- frame rendering and encoded output

## Out of scope

- full general-purpose 3D scene systems
- particles
- advanced physics
- realtime preview UI
- browser or DOM rendering
- large plugin ecosystems
- broad effect catalogs before fundamentals are stable

## Priority rule

The MVP should prefer correct semantics and deterministic output over feature count.

## Success condition

A user should be able to describe a scene, feed it to the engine, and obtain a visually credible rendered result from a CPU-first headless pipeline.
