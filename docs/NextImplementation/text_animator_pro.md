# Text Animator Pro

> **Goal:** Complete text animation capabilities with text-on-path and advanced selectors.

## Current State

Tachyon has a **solid per-glyph animation pipeline** (`TextAnimatorPipeline` with selectors, coverage, transforms).

**What exists:**
- ✅ `include/tachyon/text/animation/text_animator_pipeline.h`
- ✅ `src/text/animation/text_animator_pipeline.cpp`
- ✅ Per-glyph coverage computation
- ✅ Position, scale, rotation, opacity animation
- ✅ Fill color, stroke color, stroke width animation
- ✅ Tracking/spacing animation
- ✅ `include/tachyon/text/animation/text_on_path.h` (layout modifier exists and is wired in rendering)

**What is missing:**
- Range selectors with more advanced modes (word, line, random)
- Selector determinism contract and scene serialization

---

## 1. Glyph-Level Selectors

**Status:** ✅ **ALREADY IMPLEMENTED**

### Implemented Features
- [x] Per-glyph coverage computation
- [x] Position animation (per glyph)
- [x] Scale animation (per glyph)
- [x] Rotation animation (per glyph)
- [x] Opacity animation (per glyph)
- [x] Fill color animation
- [x] Stroke color animation
- [x] Stroke width animation
- [x] Tracking/spacing animation

### Files (Existing)
```
include/tachyon/text/animation/text_animator_pipeline.h
src/text/animation/text_animator_pipeline.cpp
include/tachyon/text/animation/text_animator_selector.h
```

---

## 2. Per-Character Animation

**Status:** ✅ **ALREADY IMPLEMENTED** (see above)

All per-glyph transforms and property animations are functional.

---

## 3. Text on Path

**Status:** ✅ Already implemented as a layout modifier, but still needs contract cleanup

### Use Cases
- Text that follows a curve (e.g., circular logo text)
- Animated text along a motion path
- Creative typography layouts

### Implementation Rules
- Implement as a **layout mode**, not as a post-warp
- Parameterize the path (bezier control points)
- Place glyphs along the path curve
- Support orientation controls (e.g., perpendicular alignment)
- Keep shaping and animation separable

### Files to Create
```
include/tachyon/text/animation/text_on_path.h    # Existing contract
src/text/animation/text_on_path.cpp              # Optional future split from header
```

### Integration Points
- Pre-animation layout step in `TextAnimatorPipeline`
- Consumes glyph placements from text layout engine
- Produces modified positions before animation applies
- Share path references through the scene contract

### Data Model (Planned)
```cpp
struct PathSpec {
    std::vector<Point2f> points; // Bezier control points
    bool closed{false};
};

class TextOnPathLayout {
public:
    // Place glyphs along path, return modified positions
    std::vector<GlyphPlacement> layout_along_path(
        const std::vector<GlyphPlacement>& base_glyphs,
        const PathSpec& path,
        float start_offset{0.0f});
};
```

---

## 4. Advanced Range Selectors

**Status:** ⚠️ Basic selectors exist, advanced modes missing

### Current State
- ✅ Basic range selector (start, end, offset)
- ✅ Per-glyph coverage based on range

### Missing Selector Modes
- [ ] **Word selector** — select by word index
- [ ] **Line selector** — select by line index
- [ ] **Random selector** — randomize order/coverage with seed
- [ ] **Wiggly selector** — noise-based coverage over time
- [ ] **Expression selector** — drive coverage via expressions
- [ ] **Deterministic serialization** — selector state must survive save/load

### Files to Modify
```
include/tachyon/text/animation/text_animator_selector.h  # ADD: new selector types
src/text/animation/text_animator_selector.cpp            # ADD: implementation
docs/NextImplementation/scene_contracts.md               # ADD: shared text path references
```

### Data Model (Planned Extension)
```cpp
enum class SelectorType {
    Range,      // Existing: start/end/offset
    Word,       // NEW: by word
    Line,       // NEW: by line  
    Random,     // NEW: random with seed
    Wiggly,     // NEW: noise over time
    Expression  // NEW: expression-driven
};

struct AdvancedSelector {
    SelectorType type;
    
    // For Range
    float start{0.0f}, end{100.0f}, offset{0.0f};
    
    // For Word/Line
    int word_index{0};
    int line_index{0};
    
    // For Random/Wiggly
    uint64_t seed{0};
    float frequency{1.0f};
    float amplitude{1.0f};
    
    // For Expression
    std::string expression_str;
    
    // Evaluate coverage for a glyph at time t
    float evaluate_coverage(int glyph_index, int total_glyphs, float time) const;
};
```

---

## Integration with Graph Editor (see `interpolation_graph.md`)

- Text animator properties (range start/end, offset) can be keyframed
- Graph editor can visualize selector animations
- Spatial paths for position animation can be edited in graph editor

---

## Implementation Priority
1. ⬜ **Text on Path** implementation (layout step)
2. ⬜ **Word selector** (commonly used in motion design)
3. ⬜ **Line selector**
4. ⬜ **Random selector** (with deterministic seed)
5. ⬜ **Wiggly selector** (time-based noise)
6. ⬜ **Expression selector** (depends on expression engine)

---

## Dependencies
- `TextAnimatorPipeline` (exists)
- Text layout engine (`src/text/layout/`)
- Bezier interpolation (`core/properties/bezier_interpolator.h`)
- Expression engine (for Expression selector)
- Graph editor backend (see `interpolation_graph.md`)
- `scene_contracts.md` for text path and selector references

---

## References
- `interpolation_graph.md` — Graph editor for text animator properties
- `camera_3d.md` — Text animators can drive camera properties via expressions
- Tracking pipeline — Text can be attached to tracked points
