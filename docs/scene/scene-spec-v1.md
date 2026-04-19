# Scene Spec v1

## Purpose

This document defines the first formal contract for authored scene data in TACHYON.
The scene spec is the reusable blueprint of a motion graphics project. It is not the same thing as a concrete render job.

## Design goals

The spec must be:

- declarative
- versioned
- explicit
- validatable
- deterministic
- reusable across many render jobs

## Canonical top-level concepts

A v1 scene spec should make room for the following categories:

- project
- compositions
- assets
- layers
- properties
- animations
- effects
- masks
- mattes
- cameras
- lights
- published template parameters

## Expected top-level shape

A future canonical structure should resemble:

- project metadata
- asset registry
- composition registry
- one entry composition or explicit target composition
- optional published property declarations

## Composition requirements

A composition should eventually define:

- id
- name
- width
- height
- fps
- duration or time domain
- background assumptions if applicable
- layer ordering model
- active camera behavior where relevant

## Asset requirements

An asset record should eventually define:

- id
- type
- source reference
- decode-time assumptions
- timing assumptions for temporal media
- fallback or error behavior

## Layer requirements

A layer should eventually define:

- id
- type
- source or generator data where needed
- timing
- visibility controls
- parent relation
- transform properties
- blend and compositing semantics
- effect stack references
- mask or matte references

## Property requirements

Properties must not be treated as untyped loose fields.
A property should eventually be defined through the property model and be compatible with:

- static values
- keyframed values
- expressions
- bound overrides

## Expressions are not the scene format

The scene spec should remain canonical authored data.
Expressions may be attached to properties, but the whole scene should not become arbitrary executable source code.

This keeps the authored contract compatible with:

- schema validation
- stable diffs
- dependency extraction
- caching
- template reuse
- clear authored versus computed boundaries

## Validation expectations

The scene spec should be strict about:

- missing references
- invalid time ranges
- invalid type assignments
- duplicate ids where uniqueness is required
- unresolved composition or asset references
- invalid published property declarations
- invalid expression references or expression source attachments

## Versioning rule

The scene spec must be explicitly versioned.
Behavior changes that affect interpretation must not hide behind unversioned defaults.

## Guiding principle

The scene spec defines what exists and how it is authored.
It should remain separate from the runtime render job that supplies concrete execution-time overrides and external data.
