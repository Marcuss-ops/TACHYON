# Template System

## Purpose

The template system turns compositions into reusable motion products instead of one-off static scenes.
This is the foundation for scalable programmatic rendering and future server-side template workflows.

## Core concept

A composition may publish a selected set of parameters as externally overridable properties.
Render jobs provide only override values instead of redefining the full scene.

## Examples of publishable parameters

- text content
- image sources
- colors
- numeric animation controls
- camera settings
- effect parameters
- shape properties

## Architectural role

The template system belongs to specification and scene binding, not to rendering itself.
It should resolve external inputs into a normal evaluated scene before rendering begins.

## Planned structure

```text
src/spec/
├── template_system.cpp
└── essential_props.cpp
```

## Design rules

1. Published properties must be explicit.
2. Overrides must remain type-safe and deterministic.
3. Template bindings must integrate with expressions, data binding, and caching.
4. Rendering should not need to know whether a value came from the original comp or a published override.
