#pragma once

#include "tachyon/tracker/algorithms/feature_tracker.h"
#include <vector>
#include <string>
#include <memory>
#include <cstdint>

namespace tachyon::ai {

// -------------------------------------------------------------------
// SegmentationProvider: abstract interface for AI-driven segmentation.
//
// This is the contract between the roto brush system and any AI model
// (SAM2, etc.).  The provider is optional: if not available, the
// engine falls back to manual bezier roto.
// -------------------------------------------------------------------
struct SegmentationMask {
    std::vector<float> alpha; // 0.0 = background, 1.0 = foreground
    uint32_t width{0};
    uint32_t height{0};

    float at(int x, int y) const {
        if (x < 0) x = 0; if (y < 0) y = 0;
        if (x >= (int)width)  x = (int)width  - 1;
        if (y >= (int)height) y = (int)height - 1;
        return alpha[y * width + x];
    }
};

struct SegmentationPrompt {
    enum Type { Point, Box, Mask } type{Point};
    // For Point: (x,y) is the point coordinate; positive=true means foreground
    float x{0}, y{0};
    float width{0}, height{0}; // For Box
    bool positive{true}; // foreground vs background
};

class SegmentationProvider {
public:
    virtual ~SegmentationProvider() = default;

    // Return true if this provider is actually available (model loaded).
    [[nodiscard]] virtual bool available() const = 0;

    // Segment a single frame given user prompts.
    // Returns an empty mask if the provider is unavailable or fails.
    virtual SegmentationMask segment_frame(
        const tracker::GrayImage& frame,
        const std::vector<SegmentationPrompt>& prompts) = 0;

    // Propagate a mask to the next frame using optical flow or model tracking.
    // Returns an empty mask if propagation fails.
    virtual SegmentationMask propagate_mask(
        const tracker::GrayImage& prev_frame,
        const tracker::GrayImage& next_frame,
        const SegmentationMask& prev_mask) = 0;

    // Provider name for diagnostics.
    [[nodiscard]] virtual std::string name() const = 0;
};

// Factory: creates the best available provider (SAM2 if compiled, else null).
std::unique_ptr<SegmentationProvider> create_segmentation_provider();

} // namespace tachyon::ai

