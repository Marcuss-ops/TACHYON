# Dependency Graph and Invalidation

## Purpose

This document defines how evaluated results depend on one another and when cached work becomes invalid.

## Core idea

TACHYON should behave as a temporal compute engine, not as a naive timeline player.
That requires an explicit dependency model.

## Responsibilities

The dependency graph should eventually describe:

- property dependencies
- parent-child transform dependencies
- composition and precomp dependencies
- matte and mask dependencies
- effect input dependencies
- template and data-binding dependencies

## Invalidation goals

When an input changes, the engine should recompute only the work whose dependencies changed.
Anything else should remain reusable.

## Cacheable categories

The engine should eventually classify what can be reused safely, such as:

- decoded assets
- text layout results
- vector preprocessing
- precomp outputs
- effect pass outputs
- evaluated property results where appropriate

## Determinism rule

Invalidation must be driven by explicit inputs and dependency edges, never by hidden mutable global state.

## Non-goal

The first version does not need a fully optimized graph engine.
It does need a documented dependency model that implementation can grow without changing semantics later.
