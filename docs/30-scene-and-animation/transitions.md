# Transitions

This document describes how transitions work today and how to add new ones without changing the existing behavior model.

## Current State

- `TransitionPresetRegistry` is intentionally empty by default.
- The transition builder functions are thin pass-through helpers.
- `build_transition_enter()` and `build_transition_exit()` only map the caller request into a `LayerTransitionSpec`.
- Empty or `none` ids mean no transition.
- Unknown ids are preserved as explicit authoring input instead of being silently rewritten.

## Data Flow

1. A caller fills `TransitionParams`.
2. The builder forwards the requested `id` to the registry.
3. The registry returns either a matching spec or a generic pass-through spec.
4. The layer stores the resulting `LayerTransitionSpec`.
5. The runtime consumes the transition exactly as authored.

## Extension Points

- `include/tachyon/presets/transition/transition_params.h`
- `include/tachyon/presets/transition/transition_builders.h`
- `src/presets/transition/transition_builders.cpp`
- `src/presets/transition/transition_presets_table.cpp`
- `src/core/transition_registry.cpp`
- `src/renderer2d/effects/core/glsl_transition_effect.cpp`

## How to Add a Transition

1. Add a transition implementation in the runtime path when the transition needs a pixel effect.
2. Register the runtime transition id in `src/core/transition_registry.cpp` if the engine needs to look it up by id.
3. Add a preset alias in `src/presets/transition/transition_presets_table.cpp` only if you want an authoring shortcut.
4. Use the explicit transition id from scene code or layer builders.
5. Keep the builder path thin.

## Rules

- Do not restore a hidden default transition.
- Do not silently map unknown ids to a visual fallback.
- Do not duplicate transition kernels in multiple registries.
- Do not change the scene authoring contract just to add a new transition name.

## Practical Result

- `none` stays `none`.
- An explicit transition id stays explicit.
- Runtime effects and authoring aliases stay separate concerns.
