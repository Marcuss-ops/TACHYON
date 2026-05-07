# Feature Extension Rules

Every new visual feature must enter through one of:

- `TransitionRegistry`
- `TransitionPresetRegistry`
- `EffectRegistry`
- `TextAnimatorPresetRegistry`
- Animation sampler / resolver

## Forbidden

- Hardcoded switch for new effect ids
- Duplicated default values
- Hidden visual fallback for unknown ids
- CLI-specific visual behavior
- Renderer-specific preset logic
- Direct feature logic inside builders
- If-else chains for feature types in renderers
- Creating new resolution paths instead of using existing resolvers

## Registry Contract

Builders must remain thin. They translate user input to registry calls only.

Aliases and authoring helpers must stay separated from runtime.

Never duplicate kernel logic in multiple registries.

## Resolver Rules

Renderers must not decide what an id means. Renderers receive already-resolved specs.

Unknown ids must produce explicit no-op, never hidden fallback visuals.

## Sampler Rules

All property sampling (scalar, vector, color, keyframes) must go through the common `property_sampler`.

No domain-specific sampler duplication.

## Testing Requirements

Every registered feature must have:
1. Registry lookup test
2. Deterministic output test
3. Schema validation test

## Extension Checklist

Before adding any feature, answer:

1. **Which registry does it enter?**
2. **Who resolves the id?**
3. **Where are the parameter defaults?**
4. **Where is the schema validated?**
5. **Which sampler is used?**
6. **Which test proves it's registered?**
7. **Which test proves it's deterministic?**

If you cannot answer all seven, the feature must not be added.
