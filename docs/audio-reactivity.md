# Audio Reactivity

## Purpose

Audio reactivity allows scene properties to respond to audio features in a deterministic way.
This is useful for music-driven motion, equalizer-like graphics, beat-reactive behavior, and template systems that derive movement from sound.

## Planned capabilities

- amplitude extraction
- band analysis
- future transient or beat detection
- exposure of audio-derived values to expressions and animation systems

## Architectural role

Audio analysis belongs to the data available during evaluation, not to the renderer itself.
The renderer consumes resolved property values. It does not analyze sound on demand.

## Planned structure

```text
src/audio/
├── audio_analysis.cpp
├── band_extractor.cpp
└── audio_driver.cpp
```

## Design rules

1. Audio-driven values must be reproducible for the same source and settings.
2. Analysis results should be cacheable.
3. Audio-derived controls should be available to expressions and published parameter workflows.
4. The system should separate analysis from visual interpretation.
