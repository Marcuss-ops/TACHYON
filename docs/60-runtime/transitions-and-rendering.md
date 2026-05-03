# Transitions and Rendering

This page is the working note for rendering transitions, especially the light leak / film burn demos.
It explains the intended data flow, the common failure modes, and the safe way to regenerate MP4s.

## Mental Model

There are three distinct concepts in Tachyon:

1. `background` - the composition clear color or base layer.
2. `layer` - the visible entity being rendered.
3. `effect` / `transition` - a post-process or layer-local operation that modifies the layer output.

Do not fuse these concepts when debugging:

- A black background is not the same thing as a black layer.
- A transition is not always a crossfade.
- A light leak is often an overlay generator, not a dissolve between two clips.

## What Was Fixed

The transition demos needed these structural fixes:

- `animated_effects` must survive compilation into the runtime scene.
- Effectful layers should render as full-frame instead of tiled blocks.
- The demo layer must be separate from the background layer.
- The light leak generator must emit black outside the active band.
- The render output must be regenerated from fresh files, not from stale cached MP4s.

Relevant files:

- `src/core/spec/compilation/scene_compiler.cpp`
- `src/core/spec/compilation/scene_compiler_build.cpp`
- `src/core/scene/evaluator/layer_evaluator.cpp`
- `src/runtime/execution/frame_executor/frame_evaluator_layer.cpp`
- `src/renderer2d/evaluated_composition/rendering/pipeline/composition_renderer.cpp`
- `src/renderer2d/effects/generators/light_leak_effect.cpp`
- `src/renderer2d/effects/core/glsl_transition_effect.cpp`
- `tests/unit/runtime/native_render_tests.cpp`

## Runtime Flow

### 1. Scene compilation

`SceneCompiler` and `SceneCompilerBuild` copy the spec layer data into the compiled scene.
If `animated_effects` are dropped here, the runtime never sees the effect animation.

### 2. Frame evaluation

The evaluator resolves:

- `start_time`
- `in_point` / `out_point`
- `opacity`
- keyframed properties
- `animated_effects`

For transition demos, the important part is that `progress` is evaluated per frame and reaches the effect kernel.

### 3. Effect application

There are two relevant effect paths:

- `src/renderer2d/effects/core/glsl_transition_effect.cpp`
- `src/renderer2d/effects/generators/light_leak_effect.cpp`

Use the transition path when the effect is meant to blend between source and destination surfaces.
Use the generator path when the effect should paint a leak over black.

## Light Leak Behavior

The current demo expects a three-part timeline:

- `0.5s` black lead-in
- `2.5s` active leak
- `0.5s` black tail-out

The generator should return the input unchanged at the very beginning and at the very end.
That keeps the black sections truly black instead of gray.

The leak itself should:

- use a diagonal projection, not a radial mask
- use a narrow band
- stay inside the active `progress` window
- use screen blending so the highlights stay bright

## Transition Rendering Rules

When authoring or debugging a transition:

- Keep the background and the animated layer separate.
- Prefer a black composition background for leak demos.
- Use a white or warm target only if you want to verify the blend path.
- Avoid `lerp`-only compositing for leak-style effects; it tends to wash the output flat.
- If you see tiled artifacts, force full-frame rendering for effectful layers.

## Demo Rendering Workflow

For the transition demos, the canonical path is the unit render test:

```powershell
.\build.ps1 -Preset dev-fast -RunTests -TestFilter native_render
```

The test writes demo MP4s under:

```powershell
build\tests\tests\output\light_leaks
```

If you need the handoff folder used by the team, copy the files to:

```powershell
output\transizioni
```

Expected demo files:

- `light_leak_demo.mp4`
- `film_burn_demo.mp4`
- `flash_demo.mp4`

## Verification Checklist

Before handing off a transition render:

- The first frame should be pure black when the effect is outside its active window.
- The middle frame should show only the beam / leak, not a flat tint.
- The output should not be tiled.
- The MP4 should be regenerated from a fresh file, not reused from an old path.

If the output still looks wrong:

- check the demo timeline first
- then check the effect kernel
- then check the compiler/runtime path for dropped animated effects
- only then touch renderer plumbing

## What Not To Do

- Do not commit generated MP4s or PNGs.
- Do not rewrite the whole renderer for a simple transition fix.
- Do not use a transition as a stand-in for a layer generator unless the runtime path explicitly supports that use case.
- Do not assume `output\transizioni` is the actual render source; it is just the copy target for the final artifacts.

