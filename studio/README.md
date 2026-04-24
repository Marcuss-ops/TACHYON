# Studio Workspace

Use `studio/library/` as the single public content catalog.

Rules:
- Keep implementation in `src/` and public contracts in `include/`.
- Do not duplicate source of truth here.
- Add authored assets only under `studio/library/`.
- Keep generation code outside the library, under `scripts/studio/`.

Public entrypoint:
- `studio/library/system/manifest.json`
