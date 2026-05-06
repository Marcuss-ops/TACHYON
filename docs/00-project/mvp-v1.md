# MVP v1 Scope

## MVP Definition

The Minimum Viable Product (v1) for Tachyon focuses on core rendering capabilities with a programmatic C++ API. To prevent scope creep, features are strictly categorized into Production Ready, Experimental, Stub, and Out of Scope.

## Production Ready (v1 Core)
These features must be stable, deterministic, and fully covered by tests for v1 release:
- **Native C++ Scene Builder**: Type-safe scene construction API.
- **2D Compositing**: Shape layers, primitive effects, and layer masking/mattes.
- **Deterministic Frame Execution**: Same code always produces the exact same video output.
- **Output Encoding**: Reliable MP4 export via FFmpeg integration.

## Experimental
These features are present but may lack full polish or optimization. They are not guaranteed to be API-stable in v1:
- **3D Bridge**: Camera models (perspective, orthographic) and basic 3D mesh support (glTF).
- **Lighting**: Basic lighting models (directional, point).
- **CPU Path Tracing**: Simplified recursive reflection implementation.

## Stubs
These are placeholders or incomplete features that are tracked but not actively developed for v1:
- **Audio Pitch Correct**: API exists but implementation is disabled or incomplete.
- **Advanced Text Layout**: RTL, vertical text, or complex shaping beyond basic word-wrapping.

## Out of Scope (Post-Scaling / v2+)
The following items are explicitly deferred to future phases (Phase 5/6) and should not block the v1 release or scaling efforts:
- Motion Blur (Sub-sampling)
- AI-assisted RotoBrush
- Physically-based physical camera
- Multi-channel Audio Mixer
- Browser-based preview & Timeline editing GUI
- Complex particle systems
- Audio reactivity

## Success Criteria

- Can render a basic 3D product promo video from C++ code
- Deterministic output (same code = same video)
- Clean C++ Builder API
- Robust CI test coverage (Smoke, Core, Render tiers)
