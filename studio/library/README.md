# Studio Library

Single source of truth for reusable public assets.

Layout:
- `animations/` for things that move
- `content/` for data only, with `lexicon/` for phrases, words, names, numbers
- `animations/typography/` for text motion packs
- `scenes/` for ready-made compositions
- `system/` for shared runtime metadata and stack helpers

Rules:
- do not duplicate content across folders
- keep `src/` and `include/` as engine implementation only
- add new public assets here, not in the engine folders
- regenerate library catalogs through the C++ scene/build pipeline, not JSON manifests
