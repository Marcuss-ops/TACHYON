# Audio-Driven Animation

## Purpose

This document defines how audio-derived features should enter TACHYON without turning the render path into an analysis-heavy black box.

## Position

Audio reactivity should be explicit, precomputed where possible, and deterministic.

The engine should consume structured audio features rather than repeatedly performing expensive or inconsistent analysis during render.

## Core model

Audio-driven animation should be based on feature extraction followed by normal property evaluation.

That means:

1. analyze audio or ingest external features
2. produce a deterministic feature stream
3. expose that stream to properties and expressions
4. evaluate properties normally over time

## Why this is preferable

This approach keeps audio logic compatible with:

- caching
- invalidation
- reproducibility
- render jobs
- debugging
- template reuse

## First supported feature families

### 1. FFT bands

Expose band amplitudes over time for low, mid, and high frequency-driven animation.

### 2. Envelope and loudness

Expose smoothed loudness or energy curves for broad motion response.

### 3. Onsets or beat events

Expose transient or beat markers for cuts, flashes, pulses, and accent timing.

### 4. Waveform-derived paths

Allow waveform samples to drive path deformation or generated graphics.

### 5. Word timing inputs

Allow forced alignment or speech timing metadata to drive subtitle and text animation.

## First expression hooks

The expression runtime should be able to query:

- a named band amplitude at time t
- current normalized loudness
- whether an onset occurred within a window
- current word timing segment where applicable

## Data model direction

Audio features should be addressable as named channels or streams, for example:

- music.low
- music.mid
- music.high
- music.loudness
- music.onset
- speech.word_index
- speech.word_progress

## Precompute preference

Whenever possible, audio features should be extracted before render and stored as structured inputs.

This is preferred over hidden realtime analysis in the render loop.

## Subtitle and speech alignment direction

Speech-driven animation should be built around external or precomputed timing data.

The engine should consume:

- token timings
- word timings
- phrase timings
- speaker segments later if needed

## Typical uses

- beat-synced transforms
- subtitle reveal by word
- per-word emphasis
- waveform graphics
- music-reactive glow or scale
- scene accents triggered by onsets

## Determinism requirements

The audio feature path must define:

- source sample rate policy
- resampling policy
- smoothing rules
- windowing rules
- normalization rules
- feature format versioning

## Non-goal

The render core should not become an always-on audio DSP workstation. The goal is deterministic feature consumption, not open-ended live audio processing.
