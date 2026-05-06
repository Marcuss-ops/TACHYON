---
Status: Canonical
Last reviewed: 2026-05-06
Owner: Core Team
Supersedes: N/A
---

# Behavior Model

Tachyon should be reasoned about with one repeated pattern across domains:

1. **Authoring intent**
   - a human chooses a name, preset, or parameters
2. **Registry resolution**
   - the name is mapped to a concrete spec
3. **Scene spec**
   - the spec stores the authored intent without executing it
4. **Compilation / evaluation**
   - the spec is normalized into runtime state
5. **Runtime consumer**
   - the appropriate subsystem performs the actual work

This is the shared model for backgrounds, transitions, text animation, and 2D effects.

## Domain Map

- **Background**
  - describes the base of a composition
  - usually becomes a layer or a resolved procedural background
- **Transition**
  - describes how a layer enters or exits
  - may be a simple state transition or a pixel kernel
- **Text animation**
  - describes how glyphs, words, or lines evolve over time
  - runs through the text animation pipeline, not the general effect host
- **2D effect**
  - describes a surface or pixel transformation
  - runs through the effect registry and effect host

## The One Question To Ask

When adding something new, ask:

- Does it change the base of the scene? -> background
- Does it change how a layer appears or disappears? -> transition
- Does it change glyphs, words, or lines? -> text animation
- Does it change pixels on a surface? -> effect

If the answer is not clear, the design is probably too broad.

## Professional Rules

- Keep presets and runtime kernels separate.
- Keep registry names explicit.
- Do not create hidden defaults that silently change behavior.
- Do not add a second pipeline when the current one can be extended.
- Prefer one canonical path per domain, not one path per feature request.
- If two names mean almost the same thing, merge them before adding more presets.

## Checklist For New Work

Before adding a new background, transition, text animation, or effect:

1. Decide which domain owns the behavior.
2. Check whether an existing registry entry already covers it.
3. Add or extend the smallest spec needed to describe it.
4. Keep the runtime kernel in the correct consumer subsystem.
5. Avoid introducing a second path for authoring the same behavior.
6. Add a focused test next to the subsystem that consumes it.
7. Confirm the naming is explicit and does not imply a different domain.

## Practical Reading Order

If you are changing one of these areas, read the relevant builder first, then the runtime consumer:

- Backgrounds: `src/presets/background/background_builders.cpp`, then `src/presets/background/background_kind_registry.cpp`
- Transitions: `src/presets/transition/transition_builders.cpp`, then `src/renderer2d/effects/core/glsl_transition_effect.cpp`
- Text: `src/presets/text/text_builders.cpp`, then `src/text/core/animation/text_animator_paints.cpp`
- 2D effects: `src/renderer2d/effects/effect_registry.cpp`, then `src/renderer2d/effects/core/effect_host.cpp`

## What This Model Prevents

- duplicated behavior hidden behind different names
- special cases that only exist in the renderer
- preset catalogs that drift away from runtime behavior
- confusion between authoring aliases and runtime kernels
