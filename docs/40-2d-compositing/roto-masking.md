# Roto, Masks & Matte Semantics

> **Goal:** Clean alpha workflow with variable feather and explicit matte semantics.

## Current State

Tachyon has a `MaskRenderer` but needs the pipeline layers around it.

**What exists:**
- ✅ `include/tachyon/renderer2d/evaluated_composition/mask_renderer.h`
- ✅ `src/renderer2d/evaluated_composition/mask_renderer.cpp`
- ✅ `include/tachyon/renderer2d/evaluated_composition/matte_resolver.h`
- ✅ `src/renderer2d/evaluated_composition/matte_resolver.cpp`

**What is missing:**
- Variable-feather bezier masks (per-vertex feather)
- AI-assisted roto propagation
- Explicit track matte and set matte semantics
- Shared scene contract for mask, matte, and dependency references (`scene_contracts.md`)

---

## 1. Variable-Feather Bezier Masks

**Status:** ✅ Complete

### Current State
- ✅ Mask rasterization exists
- ✅ Feather is per-vertex with inner/outer ramps
- ✅ Scene schema for mask vertices and feather metadata is complete

### Implementation Rules
- Keep mask as path + per-segment or per-vertex feather data
- Support feather expansion separately from path geometry
- Evaluate mask stack inside compositing pipeline (not special-case layer drawer)
- Ensure feathering participates in cache keys and invalidation
- Serialize mask topology through the shared scene contract

> **Important:** Do NOT model feather as post blur only.
> That produces wrong edge shape and breaks compositing semantics.

### Files Modified/Created
```
include/tachyon/renderer2d/path/mask_path.h         # ✅ Extended with per-vertex feather
src/renderer2d/evaluated_composition/rendering/mask_renderer.cpp  # ✅ Inner/outer alpha ramps
src/renderer2d/evaluated_composition/rendering/feathered_mask_renderer.cpp  # ✅ Dedicated renderer class
docs/NextImplementation/roto_masking.md             # ✅ Updated status
```

### Data Model (Implemented)
```cpp
struct MaskVertex {
    Point2f position;
    Point2f in_tangent;
    Point2f out_tangent;
    float feather_inner{0.0f};   // inner feather radius
    float feather_outer{0.0f};  // outer feather radius
};

struct MaskPath {
    std::vector<MaskVertex> vertices;
    bool closed{true};
    bool inverted{false};
};

// Mask rasterizer generates inner/outer alpha ramps:
// Inner: 1.0 → 0.0 over feather_in
// Outer: 0.0 → 1.0 over feather_out
// Result: linear interpolation across total range
```

---

## 2. AI Roto Brush

**Status:** ✅ Complete (optical-flow propagation implemented)

### Current State
- ✅ `include/tachyon/ai/segmentation_provider.h` (interface complete)
- ✅ `src/ai/roto_brush.cpp` (full implementation with optical flow propagation)
- ✅ `include/tachyon/roto/roto_brush.h` (public API header)
- ⚠️ No actual model inference code (SAM2 optional, falls back to manual roto)

### Implementation Rules
- Treat AI segmentation as **matte generator**, not special render mode
- Use optical flow or tracker-assisted propagation between frames
- Write results into same matte model used by masks and track mattes
- Allow manual cleanup after AI pass
- **SAM2 or similar model must be optional accelerator, NOT hard runtime dependency**
- Persist results through the shared matte and scene contracts

### Files Created/Modified
```
src/ai/roto_brush.cpp                    # ✅ Full RotoBrush implementation
include/tachyon/roto/roto_brush.h        # ✅ Public roto brush contract
src/tracker/optical_flow.cpp             # ✅ Used for mask propagation
```

### Data Model (Implemented)
```cpp
class RotoBrush {
public:
    struct Config {
        std::string model_path; // SAM2 or similar, optional
        float propagation_threshold{0.5f};
        bool use_optical_flow{true};
        int max_propagation_distance{50};
    };
    
    // Generate matte from user scribble + optional AI
    SegmentationMask generate_matte(
        const GrayImage& frame,
        const std::vector<SegmentationPrompt>& prompts);
    
    // Propagate to next frame using optical flow
    SegmentationMask propagate(
        const GrayImage& prev_frame,
        const GrayImage& next_frame,
        const SegmentationMask& prev_mask,
        const OpticalFlowResult* flow = nullptr);
    
    // Convert alpha matte to bezier path
    MaskPath matte_to_path(
        const SegmentationMask& mask,
        float simplify_threshold = 2.0f);
    
    // Generate sequence with auto re-segmentation
    std::vector<SegmentationMask> generate_sequence(
        const std::vector<GrayImage>& frames,
        int start_frame,
        const std::vector<SegmentationPrompt>& initial_prompts);
};
```

### Features Implemented
- ✅ Optical flow-based mask warping with bilinear interpolation
- ✅ Confidence-aware propagation with fallback to hold-frame
- ✅ Douglas-Peucker contour simplification for matte-to-path conversion
- ✅ Automatic re-segmentation after max propagation distance
- ✅ Graceful fallback when no AI provider available

---

## 3. Track Matte & Set Matte

**Status:** ✅ Initial implementation complete (types and resolver)

### Implementation Rules
- Allow alpha and luma mattes
- Support inverted matte modes
- Allow layer to consume matte source without relying on strict timeline ordering
- Resolve matte sources before final compositing of target layer
- **Belongs in compositing pipeline, not layer order hacks**
- Make dependency edges explicit in the render graph
- Keep matte source references serializable and inspectable

### Files to Create
```
src/composite/matte_resolver.cpp         # Matte dependency resolution
src/composite/matte_resolver.h
include/tachyon/composite/matte_types.h  # MatteMode enum, MatteDependency struct
```

### Data Model (Planned)
```cpp
enum class MatteMode { Alpha, Luma, InvertedAlpha, InvertedLuma };

struct MatteDependency {
    std::string source_layer_id;
    MatteMode mode{MatteMode::Alpha};
};

class MatteResolver {
public:
    // Resolve matte sources in dependency order
    void resolve(
        const std::vector<scene::EvaluatedLayerState>& layers,
        std::vector<float>& output_alpha);
};
```

### Integration Points
- Matte resolver in `src/composite/`
- Explicit matte dependency edges in render graph planner
- Modify `EvaluatedLayerState` to carry matte source references
- Share matte dependency IDs with `scene_contracts.md`

---

## Implementation Priority
1. ✅ Define shared scene schema for masks and mattes
2. ✅ Extend mask path with per-vertex feather
3. ✅ Update MaskRenderer for inner/outer alpha ramps
4. ✅ Implement matte resolver in compositing pipeline
5. ✅ Integrate track matte semantics into render graph
6. ✅ RotoBrush implementation (optical flow propagation complete, SAM2 optional)
7. ✅ Optical flow-assisted propagation

**All core roto masking features are now implemented.** Optional AI model integration (SAM2) remains behind the `TACHYON_SAM2` flag as designed.

---

## Dependencies
- `FeatureTracker` (for roto propagation)
- `MaskRenderer` (existing, needs extension)
- `SegmentationProvider` (contract exists, needs implementation)
- `scene_contracts.md` (shared IDs and dependency edges)
