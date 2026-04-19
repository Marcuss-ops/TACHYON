# Tachyon Engineering Spec

This document defines the engineering rules for Tachyon. The goal is a renderer that is deterministic, fast to iterate on, and explicit about what is implemented.

## 1. Ship only implemented features
- Do not expose a field, flag, or contract unless it changes pixels or behavior in the current pipeline.
- If a feature is only present in metadata, cache keys, or reports, it is not complete.
- Remove duplicate models when one model can serve the runtime directly.

## 2. Keep the render path explicit
- Color space, transfer curve, alpha mode, and motion blur must be applied in the render path, not implied by downstream tools.
- Working space is an active processing choice, not a label.
- Output transforms must happen at the output boundary.

## 3. Prefer simple ownership and data flow
- Use stack allocation by default.
- Use `std::unique_ptr` for ownership.
- Use `std::shared_ptr` only when ownership is genuinely shared.
- Avoid heap allocation in hot loops.
- Keep state passed explicitly through contexts and plans.

## 4. Optimize for the hot path
- Do not optimize code that should not exist.
- Keep data contiguous where possible.
- Prefer plain `switch` dispatch over deep inheritance for render-time work.
- Avoid hidden work inside per-layer, per-tile, or per-sample loops.

## 5. Make quality features operational
- Motion blur must use sub-frame sampling and exposure integration.
- ROI must skip work outside the active region.
- Cache policy must reflect real reuse opportunities across layers, tiles, and frames.
- Adjustment layers, mattes, precomps, and blending order must have defined runtime semantics.

## 6. Keep code small and local
- Functions should do one job.
- Large files should be split.
- Avoid deep nesting; prefer early returns.
- Use comments only for non-obvious technical decisions.
- Document difficult architectural choices in the repo, near the code they affect.

## 7. Be deterministic
- Identical inputs must produce identical frames.
- Avoid hidden mutable global state.
- Fail fast on invalid state or missing assets.
- Never silently degrade a feature into a no-op when the contract says it exists.

## 8. Use named configuration
- Replace magic numbers with named configuration.
- Render quality, tile size, recursion limits, blur samples, and similar policy values belong in configuration, not scattered literals.
- If a value affects performance or quality, it must be visible and adjustable.

## Non-negotiables
- No fake features.
- No silent fallbacks that mask missing implementation.
- No ambiguous ownership.
- No hidden color transforms.
- No render-time allocation spikes.

