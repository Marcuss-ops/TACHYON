# CPU Path Tracing Direction

## Purpose

This document defines the intended role of a CPU-first physically based renderer inside TACHYON.

## Position

A CPU path tracer is a strategic expansion of TACHYON, not a replacement for the core compositing engine.

The goal is to support serious 3D rendering without requiring GPUs, browser runtimes, or workstation-only infrastructure.

## Why a CPU path tracer fits TACHYON

A CPU-first path tracer aligns with the project direction because it offers:

- deterministic execution
- strong batch rendering behavior
- good scaling across cores
- no mandatory GPU dependency
- predictable server deployment
- natural support for offline quality features

The intended implementation should pair Embree for BVH and ray intersection with a compact path integrator and optional Open Image Denoise for low-sample output.

## Intended use cases

The path tracing backend should primarily serve:

- 3D logo renders
- title cards with extruded text
- product shots
- simple motion design scenes with lights and cameras
- 3D inserts inside otherwise 2D compositions
- reusable 3D precomps

## Features that become natural in this backend

A physically based backend makes the following features first-class instead of ad hoc:

- reflections
- refractions
- soft shadows
- area lights
- image-based lighting
- depth of field
- physically grounded motion blur
- indirect lighting

## Scope discipline

The first 3D backend should not try to become a full DCC application.

It only needs to support the subset of 3D features that materially improve automated video rendering.

## First supported asset scope

The first 3D asset scope should be:

- procedural primitives if needed
- extruded text and shapes later
- glTF import for mesh assets
- PBR material inputs kept small and explicit

## Realistic feature cut

The backend should stay aggressively focused. The target is not "everything 3D", but the subset that gives the biggest quality gain for YouTube-style and cinematic clips.

Keep in scope early:

- mesh layers
- extruded text
- extruded shapes
- perspective cameras
- point, spot, directional, and area lights
- HDRI environment lighting
- motion blur
- depth of field
- ray-traced shadows, reflections, refractions, and GI

Defer until later:

- volumetrics
- hair
- cloth
- particles
- large procedural material graphs
- full simulation stacks

## First material scope

The first material model should remain compact:

- base color
- metallic
- roughness
- normal
- emissive
- opacity or transmission where needed

Anything more advanced should wait until pass contracts and testing are solid.

## First lighting scope

The first lighting scope should include:

- directional lights
- point lights
- spot lights
- environment lighting via HDRI

Area lights are valuable, but can wait if implementation pressure becomes too high.

## First camera scope

The first camera scope should include:

- perspective camera
- focal length
- aperture or f-stop model
- focus distance
- shutter interval for motion blur

## Render pass contract

The path tracing backend should render into explicit passes rather than directly owning final output.

At minimum it should be able to emit:

- beauty
- alpha
- depth
- normal
- albedo
- motion vectors

The first denoise-ready contract should also keep enough auxiliary data around for OIDN-style reconstruction when the pipeline wants it.

## Denoising position

Denoising should be treated as an explicit post-render stage with versioned inputs and settings.

It should never be hidden or silently applied.

## Sampling policy

The engine should expose explicit controls for:

- samples per pixel
- max bounce count
- Russian roulette policy where relevant
- denoise enabled or disabled
- adaptive sampling later if needed

## Determinism expectations

The path tracing path must define:

- seed policy
- sample ordering policy
- accumulation policy
- floating-point stability expectations
- denoise model versioning

## Recommended milestone placement

CPU path tracing should not be part of the smallest engine MVP.

It should land after:

- scene spec basics
- composition rendering
- text and shape reliability
- effect hosting baseline
- deterministic compositing baseline

A good mental model is:

- MVP v1: motion graphics engine becomes real
- MVP v1.5 or v2: 3D physically based rendering becomes real
