#include "tachyon/ai/segmentation_provider.h"
#include "tachyon/tracker/feature_tracker.h"
#include <algorithm>
#include <cmath>

namespace tachyon::ai {

// ---------------------------------------------------------------------------
// RotoBrush: AI-assisted matte generation with manual fallback.
//
// Workflow:
//   1. User provides prompts (points, boxes) on a reference frame.
//   2. If an AI provider is available, segment the reference frame.
//   3. Propagate the mask forward/backward using optical flow or
//      the provider's tracking.
//   4. Output a sequence of matte frames that can be consumed by
//      the compositing pipeline as a track matte source.
// ---------------------------------------------------------------------------

struct RotoBrushResult {
    std::vector<SegmentationMask> frames;
    int start_frame{0};
    int end_frame{0};
    bool ai_assisted{false};
};

class RotoBrush {
public:
    explicit RotoBrush(std::unique_ptr<SegmentationProvider> provider)
        : m_provider(std::move(provider)) {}

    // Segment a single frame from user prompts.
    SegmentationMask segment(
        const tracker::GrayImage& frame,
        const std::vector<SegmentationPrompt>& prompts) {
        if (m_provider && m_provider->available()) {
            return m_provider->segment_frame(frame, prompts);
        }
        return {};
    }

    // Propagate a mask to the next frame.
    // Uses the AI provider if available, otherwise falls back to
    // optical-flow-based diffusion (placeholder).
    SegmentationMask propagate(
        const tracker::GrayImage& prev_frame,
        const tracker::GrayImage& next_frame,
        const SegmentationMask& prev_mask) {
        if (m_provider && m_provider->available()) {
            return m_provider->propagate_mask(prev_frame, next_frame, prev_mask);
        }
        // Fallback: simple optical-flow-based mask warp (not implemented here)
        // In a full implementation, this would use the FeatureTracker's LK flow
        // to warp the mask pixels.
        return prev_mask; // hold frame as fallback
    }

    // Generate a matte sequence for a frame range.
    // If AI is unavailable, returns empty result (user must do manual roto).
    RotoBrushResult generate_sequence(
        const std::vector<tracker::GrayImage>& frames,
        int start_frame,
        const std::vector<SegmentationPrompt>& initial_prompts) {
        RotoBrushResult result;
        if (frames.empty()) return result;

        result.start_frame = start_frame;
        result.end_frame = start_frame + static_cast<int>(frames.size()) - 1;
        result.ai_assisted = (m_provider && m_provider->available());

        if (!result.ai_assisted) {
            return result; // empty frames = manual fallback
        }

        // Segment reference frame
        auto mask = m_provider->segment_frame(frames[0], initial_prompts);
        if (mask.alpha.empty()) {
            return result;
        }
        result.frames.push_back(std::move(mask));

        // Propagate forward
        for (size_t i = 1; i < frames.size(); ++i) {
            mask = propagate(frames[i - 1], frames[i], result.frames.back());
            result.frames.push_back(std::move(mask));
        }

        return result;
    }

    [[nodiscard]] bool has_provider() const {
        return m_provider && m_provider->available();
    }

    [[nodiscard]] std::string provider_name() const {
        return m_provider ? m_provider->name() : "none";
    }

private:
    std::unique_ptr<SegmentationProvider> m_provider;
};

} // namespace tachyon::ai
