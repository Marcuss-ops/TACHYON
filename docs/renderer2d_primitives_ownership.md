# Renderer2D Primitives Ownership

This document defines the canonical ownership of Renderer2D primitive operations.

## Ownership Map

| Domain | Responsibility |
|--------|-----------------|
| `renderer2d/sampling` | Sampling/resampling operations |
| `renderer2d/blend` | Blend modes (normal, multiply, screen, etc.) |
| `renderer2d/mask` | Matte/mask resolve operations |
| `renderer2d/effects` | Effects (blur, LUT, vignette, etc.) |
| `renderer2d/composite` | Layer compositing operations |
| `renderer2d/surface` | SurfaceRGBA ownership |

## Forbidden Patterns

The following patterns must NOT appear outside the owning domain:

- `gaussian_blur` - Use `renderer2d/effects/gaussian_blur.h`
- `blend_normal`, `blend_multiply`, etc. - Use `renderer2d/blend/blend_modes.h`
- `alpha_over`, `composite_over` - Use `renderer2d/composite/compositor.h`
- `sample_bilinear`, `sample_nearest` - Use `renderer2d/sampling/sampler.h`

## Guardrail Script

Run the following to check for violations:

```powershell
.\scripts\primitive-check.ps1
```

This script will:
1. Scan for forbidden patterns outside owner domains
2. Report violations with file paths
3. Exit with error if violations found (CI failure)

## Allowlist

In rare cases, a domain may need to implement a localized version of a primitive.
To allowlist a file, add a comment:

```cpp
// PRIMITIVE_ALLOWLIST: gaussian_blur - reason: specialized fast path for text rendering
```

The guardrail script will skip files with this comment.
