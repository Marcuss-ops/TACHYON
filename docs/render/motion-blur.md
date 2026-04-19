# Motion Blur

## Purpose

This document defines motion blur as an explicit render feature rather than a vague visual enhancement.

Motion blur is one of the strongest contributors to cinematic credibility in motion graphics and composited video.
If TACHYON wants to feel closer to a serious compositing engine than to generic generated graphics, motion blur must be treated as a first-class time-sampling system.

## Why it matters

Motion blur is not only about softness.
It communicates exposure over time.
It affects how fast motion, camera movement, transforms, and 3D motion read to the eye.

Weak or fake motion blur often makes scenes feel synthetic even when the rest of the render is technically correct.

## Core rule

Motion blur should come from sub-frame evaluation across an exposure interval.
It should not default to a cheap screen-space smear or a post-process guess.

## Scope

The motion blur system should eventually support:

- 2D layer motion blur
- camera motion blur
- 3D object motion blur
- transform-driven blur
- optional deformation-aware blur later
- deterministic sampling across the shutter interval

## Motion sources

The first implementation should explicitly support blur from:

- position changes
- scale changes where resampling matters
- rotation changes
- anchor-related motion
- parent chain motion
- camera movement
- 3D object motion

Later support may include:

- deformation blur
- path-shape topology changes where the engine can do so correctly
- material or shading changes only where this is semantically justified

## Shutter model

Motion blur should be controlled through an explicit shutter model.

At minimum, the system should define:

- shutter enabled or disabled
- shutter angle
- shutter phase or centered exposure policy
- sample count

## Shutter angle

Shutter angle should define the temporal exposure fraction relative to one frame interval.

Recommended first semantics:

- 0 means no motion blur
- 180 means half-frame exposure
- 360 means full-frame exposure

The engine should document the exact time interval derived from the chosen angle and frame rate.

## Exposure placement

The engine should choose and document one initial exposure placement policy.

Recommended first policy:

- centered exposure around the nominal frame time

That means the exposure interval straddles the frame sample time symmetrically.
This is predictable and suitable for compositing semantics.

## Sample count

Motion blur should use configurable sub-frame sample counts.

Recommended first policy:

- composition default sample count
- optional per-layer override later
- deterministic sample positions for the same render job

Suggested first quality tiers:

- draft: 2 or 4 samples
- high: 8 samples
- cinematic: 12 or 16 samples where justified

The exact defaults can be tuned later, but the contract should reserve the concept now.

## Deterministic sampling

Sample positions inside the shutter interval must be deterministic.

That means:

- the same frame, shutter, and sample count produce the same sub-frame times
- no hidden random jitter unless explicitly part of a deterministic seeded policy
- sampling layout must be part of visible-output compatibility behavior

## Layer-level controls

Motion blur should not be global only.
It should support per-layer participation.

Recommended first layer controls:

- inherit composition setting
- force off
- force on
- blur multiplier later if needed

This gives enough control for practical scenes without bloating the first version.

## Composition-level controls

The composition should define the baseline motion blur policy.

Recommended first composition controls:

- motion blur enabled or disabled
- shutter angle
- sample count
- quality tier or sampling policy reference later if needed

## 2D integration

For 2D layers, motion blur should come from repeated evaluation and rendering across sub-frame times.

That means the engine should:

- evaluate properties at each sub-frame time
- resolve transforms at each sample
- render and accumulate with explicit weighting
- respect masks, mattes, effects, and compositing semantics at those sub-frame states where required

The first version may apply some restrictions for especially expensive effects, but those restrictions must be explicit.

## 3D integration

For the CPU 3D path, motion blur should integrate with the renderer's time sampling model.

That means:

- camera and object transforms should be sampleable over the shutter interval
- path-traced or ray-traced frames should accumulate over those sub-frame times
- any seeded stochastic behavior must remain deterministic for a given job configuration

## Caching implications

Motion blur must not force the engine into all-or-nothing cache invalidation.

The cache model should treat motion blur as part of render identity.
At minimum, cache keys should include:

- frame time
- shutter angle or interval policy
- sample count
- layer motion blur participation where relevant
- any deterministic sampling pattern identifier

## Caching strategy direction

The first implementation should prefer explicit sample-aware caching units rather than pretending blurred frames are the same as non-blurred frames.

Useful cacheable units may include:

- evaluated sub-frame property states
- stable precomp sample results where valid
- expensive effect results when the sampled input state is identical
- accumulated frame buffers only when the full dependency set matches

## Invalidation rule

A motion-blurred frame should invalidate only when one of its real dependencies changes.
The existence of motion blur must not automatically destroy all upstream reuse.

## ROI and performance

On weak machines, motion blur can become one of the most expensive features in the entire engine.

The engine should therefore leave room for:

- region-of-interest aware sampling
- sample count tiers
- per-layer participation controls
- selective disabling for draft previews or low-priority outputs
- explicit profiling of motion blur cost versus the rest of the frame

## Interaction with effects

Motion blur interacts with effects in ways that must be documented.

The first implementation should clearly define whether an effect is:

- sampled inside each sub-frame render
- applied after temporal accumulation
- unsupported for accurate blur in the first version

This matters especially for:

- glow
- directional or radial style blurs
- displacement-based effects
- shadow effects
- depth-based effects

## Interaction with text and shapes

Text and shape layers should participate in motion blur through the same sub-frame evaluation model.

That means:

- per-character animation can blur if the animated properties change over the shutter interval
- trim paths and repeater-driven motion can blur through repeated evaluation
- sharp vector appearance should remain consistent apart from intentional temporal accumulation

## Compatibility rule

Changes to shutter semantics, sample placement, or accumulation policy are visible-output changes.
They should be treated as compatibility-significant behavior.

## First implementation recommendation

The first serious motion blur milestone should target:

- composition-level enable toggle
- composition-level shutter angle
- composition-level sample count
- layer participation on or off
- centered exposure policy
- deterministic sub-frame evaluation
- accumulation in the 2D compositor
- basic camera and object blur in the 3D path where available

## Not first-version priorities

The following should not block the first motion blur release:

- deformation blur for every primitive type
- per-property blur weighting
- rolling shutter simulation
- vector-pass reconstruction blur as the primary model
- highly specialized cinematic shutter models

## Guiding principle

TACHYON should treat motion blur as a time-evaluation feature, not a decorative post effect.
That is how the engine gets closer to serious compositing quality while staying deterministic and CPU-first.
