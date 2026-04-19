# Data Binding

## Purpose

Data binding turns TACHYON from a static scene renderer into a programmatic media engine that can produce variations from structured inputs.

## Supported source categories

The architecture should leave room for structured external sources such as:

- JSON
- CSV
- future table-like or API-fed sources outside the core engine

## Core concept

External data should be bindable to scene properties, published template values, text content, image choices, effect parameters, and other controlled inputs.

## Architectural role

Data binding belongs before rendering and before final property evaluation.
It resolves external data into scene-visible values that downstream systems treat as normal inputs.

## Planned structure

```text
src/data/
├── csv_source.cpp
├── json_source.cpp
└── data_binding.cpp
```

## Design rules

1. Data binding must remain explicit and type-aware.
2. Bound values must integrate with templates, expressions, and caching.
3. The renderer should not need special knowledge of external data sources.
4. Data-driven rendering must remain deterministic for the same source snapshot.
