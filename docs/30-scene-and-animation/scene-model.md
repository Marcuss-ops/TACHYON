# Scene Model

Tachyon's scene model is a hierarchical graph of layers and nodes.

## Core Concepts

- **Composition**: The root container for a render job.
- **Layer**: A single visual or logical element in time.
- **Node**: A property-bearing entity within a layer.
- **Property**: An animatable value (Color, Vector, Scalar).
- **Animation**: Temporal functions (Keyframes, Expressions) that drive properties.

## Hierarchies

- **Parenting**: Layers can be parented to others, inheriting transformations.
- **Pre-compositions**: Nested compositions for modularity.
