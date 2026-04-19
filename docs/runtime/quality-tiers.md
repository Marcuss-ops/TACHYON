# Quality Tiers

## Purpose

This document defines the explicit quality tiers that TACHYON should expose.

It exists to prevent quality behavior from becoming hidden or ad hoc.
A programmable engine should let jobs ask for a named quality tier with predictable consequences.

## Core rule

A quality tier may change cost and performance characteristics, but it must not silently invent new scene semantics.

That means:
- a lower tier may reduce sample counts or disable optional expensive features if the tier contract says so
- a tier must not reinterpret transforms, timing, or compositing rules
- visible changes introduced by a tier must be intentional and documented

## Why this matters

Quality tiers help:
- weak machines survive heavy jobs
- automation systems choose the right cost level
- debugging and preview workflows move faster
- output profiles and render jobs stay smaller and cleaner

## Initial tier set

Recommended first tiers:
- draft
- high
- cinematic

## Draft tier

Purpose:
- fast validation
- iteration and preview
- low-cost automation where final polish is not required

Typical characteristics:
- lower motion-blur sample count
- reduced 3D sample count
- optional lower-cost effect paths where explicitly supported
- proxy or lower-cost media handling where allowed by the workflow
- smaller queue and buffer expectations where useful

## High tier

Purpose:
- normal production output
- strong balance of quality and cost

Typical characteristics:
- standard motion-blur sampling
- normal 2D compositing quality
- denoised low-sample 3D where appropriate
- standard delivery-focused defaults

## Cinematic tier

Purpose:
- highest approved quality for jobs that justify the cost

Typical characteristics:
- higher motion-blur sample count
- higher 3D sample count
- stronger accumulation where required
- larger quality-oriented buffer or surface expectations if necessary
- output settings chosen for richer delivery or master use cases

## Tier-controlled knobs

The engine should define which knobs a tier is allowed to influence.

Recommended first tier-controlled categories:
- motion blur sample count
- 3D sample count
- optional denoise enable by profile or job policy
- effect quality policy only where the effect explicitly supports tiers
- buffering and queue defaults when operationally useful

## Knobs that tiers should not redefine

A tier should not silently change:
- layer timing semantics
- alpha model
- color working space semantics
- compositing order
- deterministic seed identity unless the tier is part of job identity

## Job identity rule

Selected quality tier should be part of render-job identity whenever it can alter visible output.

That means a draft render and a cinematic render are not the same job outcome even if everything else matches.

## Output-profile interaction

Quality tier and output profile are related but distinct.

- quality tier controls how the scene is rendered
- output profile controls how the result is delivered or encoded

A delivery profile may still be rendered at cinematic quality.
A master profile may still be rendered at high quality.
The engine should keep these axes separate.

## Operational defaults

Recommended first behavior:
- if a job omits quality tier, choose a documented default such as `high`
- no hidden environment-specific default changes
- CLI and APIs should expose the chosen tier clearly in logs and summaries

## First implementation recommendation

The first implementation should support:
- draft
- high
- cinematic
- explicit mapping from tier to motion blur sample count
- explicit mapping from tier to 3D sample count
- visible reporting of the active tier in job summaries

## Guiding principle

Quality tiers should make TACHYON easier to operate, not harder to trust.
They should reduce cost in explicit, documented ways while preserving the engine's core semantics.
