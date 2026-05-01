# Golden Tests

Golden tests are visual regression tests that compare rendered output against known-good reference images (golden masters).

## Purpose

- Detect visual regressions automatically
- Validate rendering determinism
- Provide visual proof of feature correctness

## How Golden Tests Work

1. **Render** a test scene to an image/video
2. **Compare** output against the golden master
3. **Pass** if identical (within tolerance)
4. **Fail** if different (and update golden if change is intentional)

## Test Structure

```
tests/
├── golden/
│   ├── scenes/          # Test scene definitions (C++ or JSON)
│   │   ├── shape_basic.cpp
│   │   ├── text_basic.cpp
│   │   └── ...
│   └── golden_hashes.json  # Hash registry for golden images
└── unit/
    └── runtime/
        └── visual/
            ├── visual_golden_tests.cpp
            └── golden_fixtures.cpp
```

## Adding a Golden Test

### 1. Create a Test Scene

```cpp
// tests/golden/scenes/my_feature.cpp
#include "tachyon/scene/builder.h"

void register_my_feature_scene() {
    auto scene = tachyon::scene::SceneBuilder()
        .project("my_feature", "My Feature Test")
        .composition("main", [](auto& comp) {
            comp.size(1920, 1080).fps(30).duration(1.0);
            comp.shape_layer("test", [](auto& l) {
                l.rectangle(100, 100, 500, 500).fill_color(1.0, 0.0, 0.0);
            });
        })
        .build();
    tachyon::register_golden_scene("my_feature", scene);
}
```

### 2. Register the Test

In `golden_fixtures.cpp`:
```cpp
extern void register_my_feature_scene();
// ...
void register_all_golden_scenes() {
    // ...
    register_my_feature_scene();
}
```

### 3. Generate Golden Master

Run with generation mode:
```bash
.\build.ps1 -Preset dev -RunTests -TestFilter golden
# Or set environment variable
$env:TACHYON_GENERATE_GOLDEN=1
.\build.ps1 -Preset dev -RunTests -TestFilter golden
```

### 4. Verify Test Passes

```bash
.\build.ps1 -Preset dev -RunTests -TestFilter golden
```

## Golden Hash Registry

The file `tests/golden/golden_hashes.json` stores expected hashes:

```json
{
  "shape_basic": "abc123def456...",
  "text_basic": "789xyz...",
  "my_feature": "newhash..."
}
```

Update this file when intentional visual changes are made.

## Visual Diff on Failure

When a golden test fails:
1. The test outputs the rendered image to `tests/golden/actual/`
2. The golden master is in `tests/golden/scenes/` (or expected location)
3. Compare visually or use image diff tools

## CI Integration

Golden tests run in CI (see `.github/workflows/ci.yml`):
- Linux: `ctest -R golden`
- Windows: `ctest -R golden`

Failed golden tests block merge until either:
- The regression is fixed, or
- The golden hash is updated (after review)
