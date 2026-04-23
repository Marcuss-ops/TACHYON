#include "tachyon/ai/segmentation_provider.h"
#include <memory>

namespace tachyon::ai {

// ---------------------------------------------------------------------------
// Null provider: always unavailable.  Used when no AI backend is compiled.
// ---------------------------------------------------------------------------
class NullSegmentationProvider : public SegmentationProvider {
public:
    [[nodiscard]] bool available() const override { return false; }

    SegmentationMask segment_frame(
        const tracker::GrayImage&,
        const std::vector<SegmentationPrompt>&) override {
        return {};
    }

    SegmentationMask propagate_mask(
        const tracker::GrayImage&,
        const tracker::GrayImage&,
        const SegmentationMask&) override {
        return {};
    }

    [[nodiscard]] std::string name() const override { return "null"; }
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------
std::unique_ptr<SegmentationProvider> create_segmentation_provider() {
#ifdef TACHYON_SAM2
    // If SAM2 is compiled in, try to create it first
    extern std::unique_ptr<SegmentationProvider> create_sam2_provider();
    auto sam2 = create_sam2_provider();
    if (sam2 && sam2->available()) {
        return sam2;
    }
#endif
    return std::make_unique<NullSegmentationProvider>();
}

} // namespace tachyon::ai
