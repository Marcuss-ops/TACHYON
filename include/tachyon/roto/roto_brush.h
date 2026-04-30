#pragma once

#include "tachyon/ai/segmentation_provider.h"
#include "tachyon/tracker/optical_flow.h"
#include "tachyon/renderer2d/path/mask_path.h"
#include <vector>
#include <memory>
#include <string>

namespace tachyon::ai {

/**
 * @brief RotoBrush: AI-assisted matte generation with manual fallback.
 * 
 * Workflow:
 *   1. User provides prompts (points, boxes) on a reference frame.
 *   2. If an AI provider is available, segment the reference frame.
 *   3. Propagate the mask forward/backward using optical flow or
 *      the provider's tracking.
 *   4. Output a sequence of matte frames that can be consumed by
 *      the compositing pipeline as a track matte source.
 */
class RotoBrush {
public:
    struct Config {
        std::string model_path;           // SAM2 or similar, optional
        float propagation_threshold{0.5f}; // Minimum confidence for propagation
        bool use_optical_flow{true};       // Use optical flow for propagation
        int max_propagation_distance{50};  // Max frames to propagate without re-segmentation
    };

    explicit RotoBrush(std::unique_ptr<SegmentationProvider> provider = nullptr);
    ~RotoBrush() = default;

    // Configure the roto brush
    void set_config(const Config& config) { config_ = config; }
    const Config& config() const { return config_; }

    /**
     * @brief Generate matte from user scribble + optional AI.
     * @param frame Input grayscale frame
     * @param prompts User prompts (points, boxes)
     * @return Segmentation mask (alpha matte)
     */
    SegmentationMask generate_matte(
        const tracker::GrayImage& frame,
        const std::vector<SegmentationPrompt>& prompts);

    /**
     * @brief Propagate mask to next frame using optical flow.
     * @param prev_frame Previous grayscale frame
     * @param next_frame Next grayscale frame
     * @param prev_mask Previous segmentation mask
     * @param flow_result Optional pre-computed optical flow (if null, will compute)
     * @return Propagated segmentation mask
     */
    SegmentationMask propagate(
        const tracker::GrayImage& prev_frame,
        const tracker::GrayImage& next_frame,
        const SegmentationMask& prev_mask,
        const tracker::OpticalFlowResult* flow_result = nullptr);

    /**
     * @brief Convert a propagated alpha matte to a bezier mask path.
     * @param mask Alpha matte to convert
     * @param simplify_threshold Threshold for path simplification (pixels)
     * @return Approximate bezier mask path
     */
    renderer2d::MaskPath matte_to_path(
        const SegmentationMask& mask,
        float simplify_threshold = 2.0f);

    /**
     * @brief Generate a sequence of masks for a frame range.
     * @param frames Sequence of grayscale frames
     * @param start_frame Starting frame index
     * @param initial_prompts Initial user prompts on first frame
     * @return Sequence of segmentation masks
     */
    std::vector<SegmentationMask> generate_sequence(
        const std::vector<tracker::GrayImage>& frames,
        int start_frame,
        const std::vector<SegmentationPrompt>& initial_prompts);

    /**
     * @brief Check if AI provider is available.
     */
    [[nodiscard]] bool has_provider() const;

    /**
     * @brief Get provider name for diagnostics.
     */
    [[nodiscard]] std::string provider_name() const;

private:
    std::unique_ptr<SegmentationProvider> m_provider;
    Config m_config;
    tracker::OpticalFlowCalculator m_flow_calculator;
    
    // Internal helper: warp mask using optical flow vectors
    SegmentationMask warp_mask_with_flow(
        const SegmentationMask& prev_mask,
        const tracker::OpticalFlowResult& flow,
        int width, int height);
};

} // namespace tachyon::ai