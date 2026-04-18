# Non-goals

This document exists to protect TACHYON from becoming vague, bloated, or misaligned.

## TACHYON is not

### Not a browser renderer
It will not depend on Chromium, DOM layout, CSS rendering, or browser automation in the render path.

### Not a web-first developer framework
The project is not being shaped around React-like abstractions, TypeScript-first scene execution, or browser-style composition models.

### Not a GUI editor
The first goal is not to build a timeline editor, viewport application, or desktop creative suite.
Tooling may come later, but the engine comes first.

### Not a clone of Premiere Pro
Premiere is an editing application. TACHYON is a render engine and compositing core.
Some concepts may overlap, but product goals differ.

### Not a clone of After Effects
After Effects is a large creative application with extensive interactive tooling.
TACHYON should learn from strong internal concepts like compositions, cameras, layers, and precomps without inheriting unnecessary product complexity.

### Not an orchestration platform
It should not own AI prompting, remote job scheduling, media acquisition, publishing workflows, or application business logic.

### Not a general-purpose game engine
Even though it may contain scene graphs, cameras, matrices, and 3D transforms, the target is motion graphics and compositing, not games.

### Not architecture driven by demos
No feature should exist just because it looks impressive in a demo.
Foundational systems come before showcase features.

## Anti-patterns to avoid

- putting parsing logic inside render loops
- mixing scene evaluation with pixel output code
- treating parallax as an effect instead of a result of projection
- embedding camera behavior inside arbitrary UI tooling
- building broad feature surfaces before profiling and testability exist
- creating a monolithic renderer with no separation between scene, camera, compositing, and encoding

## Scope discipline

The project should grow by strengthening core systems, not by accumulating disconnected features.
When in doubt, prefer stronger foundations over broader surface area.
