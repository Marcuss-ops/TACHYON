# Rendering Contract

## Purpose

This document defines the baseline visual semantics TACHYON should preserve across implementations.

## Why it matters

A motion graphics engine is not judged only by whether it renders pixels.
It is judged by whether those pixels are stable, predictable, and visually credible.

## Baseline contract

The engine should explicitly define:

- alpha representation
- blend mode semantics
- image sampling rules
- subpixel behavior
- transform resampling policy
- motion blur policy
- antialiasing policy
- precision expectations

## Alpha

The engine should choose one internal alpha model and document where conversions occur.
Premultiplied versus straight alpha must never remain implicit.

## Blending

Blend mode behavior should be deterministic and documented.
The first implementation should prioritize a small reliable subset over a large ambiguous one.

## Sampling

Sampling defaults for scale, rotation, and subpixel translation must be explicit.
The engine should avoid hidden quality shifts caused by backend differences.

## Motion blur

Motion blur must be an explicit evaluated feature, not an accidental byproduct.
It should be controllable by composition and layer-level policy.

## Antialiasing

Vector edges, text edges, and transformed imagery require a documented antialiasing policy.
The first version should favor consistent quality over aggressive optimization.

## Precision

The minimum accepted internal precision for transforms, timing, blending, and color should be documented so deterministic output remains believable across scenes.

## Non-goal

The first rendering contract does not need every future cinema-grade feature.
It needs a clear, reliable baseline that implementation and tests can target.
