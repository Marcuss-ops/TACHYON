#include "tachyon/timeline/time_remap.h"
#include <algorithm>
#include <cmath>

namespace tachyon::timeline {

// TimeRemapEvaluator implementation
TimeRemapEvaluator::TimeRemapEvaluator(const Config& config) : config_(config) {}

float TimeRemapEvaluator::evaluate(const TimeRemapCurve& curve, float dest_time, float frame_duration) const {
    (void)frame_duration;
    return evaluate_source_time(curve, dest_time);
}

FrameBlendResult TimeRemapEvaluator::evaluate_with_flow(
    const TimeRemapCurve& curve,
    float dest_time,
    float frame_duration,
    const tracker::OpticalFlowResult* flow_result) const {
    
    FrameBlendResult result = evaluate_frame_blend(curve, dest_time, frame_duration);
    
    // If optical flow is enabled and we have flow data, adjust blend based on confidence
    if (config_.enable_optical_flow_warping && flow_result && flow_result->valid_count() > 0) {
        const float avg_confidence = flow_result->average_confidence();
        
        // If confidence is low, fall back to linear blend
        if (avg_confidence < config_.optical_flow_confidence_threshold) {
            // Reduce blend factor proportionally to confidence
            result.blend_factor *= avg_confidence / config_.optical_flow_confidence_threshold;
        }
    }
    
    return result;
}

std::vector<float> TimeRemapEvaluator::warp_frame(
    const std::vector<float>& frame,
    int width, int height, int channels,
    const tracker::OpticalFlowResult& flow,
    float warp_factor) const {
    
    if (!config_.enable_optical_flow_warping || frame.empty()) {
        return frame;
    }
    
    std::vector<float> warped(frame.size(), 0.0f);
    (void)width;
    (void)height;
    
    // Warp each pixel using optical flow vectors
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int idx = y * width + x;
            const auto* vec = flow.get_flow_at_pixel(x, y);
            
            if (!vec || !vec->valid) {
                // Copy original pixel if no valid flow
                for (int c = 0; c < channels; ++c) {
                    warped[idx * channels + c] = frame[idx * channels + c];
                }
                continue;
            }
            
            // Apply flow with warp factor
            const float src_x = static_cast<float>(x) - vec->flow.x * warp_factor;
            const float src_y = static_cast<float>(y) - vec->flow.y * warp_factor;
            
            // Bilinear interpolation
            const int x0 = static_cast<int>(std::floor(src_x));
            const int y0 = static_cast<int>(std::floor(src_y));
            const int x1 = x0 + 1;
            const int y1 = y0 + 1;
            
            const float dx = src_x - static_cast<float>(x0);
            const float dy = src_y - static_cast<float>(y0);
            
            // Clamp to bounds
            const int x0_clamped = std::clamp(x0, 0, width - 1);
            const int y0_clamped = std::clamp(y0, 0, height - 1);
            const int x1_clamped = std::clamp(x1, 0, width - 1);
            const int y1_clamped = std::clamp(y1, 0, height - 1);
            
            for (int c = 0; c < channels; ++c) {
                const float v00 = frame[(y0_clamped * width + x0_clamped) * channels + c];
                const float v10 = frame[(y0_clamped * width + x1_clamped) * channels + c];
                const float v01 = frame[(y1_clamped * width + x0_clamped) * channels + c];
                const float v11 = frame[(y1_clamped * width + x1_clamped) * channels + c];
                
                // Bilinear interpolation
                const float v0 = v00 * (1.0f - dx) + v10 * dx;
                const float v1 = v01 * (1.0f - dx) + v11 * dx;
                warped[idx * channels + c] = v0 * (1.0f - dy) + v1 * dy;
            }
        }
    }
    
    return warped;
}

float evaluate_source_time(const TimeRemapCurve& curve, float dest_time) {
    if (curve.keyframes.empty()) {
        return dest_time;
    }

    // Binary search for the segment
    auto it = std::lower_bound(curve.keyframes.begin(), curve.keyframes.end(), std::make_pair(0.0f, dest_time),
        [](const std::pair<float, float>& a, const std::pair<float, float>& b) {
            return a.second < b.second;
        });

    if (it == curve.keyframes.begin()) {
        return it->first;
    }
    if (it == curve.keyframes.end()) {
        return curve.keyframes.back().first;
    }

    auto prev = std::prev(it);
    
    // Linear interpolation between (prev_src, prev_dest) and (curr_src, curr_dest)
    float t = (dest_time - prev->second) / (it->second - prev->second);
    
    if (curve.mode == TimeRemapMode::Hold) {
        return prev->first;
    }
    
    return prev->first + t * (it->first - prev->first);
}

FrameBlendResult evaluate_frame_blend(
    const TimeRemapCurve& curve, 
    float dest_time, 
    float frame_duration) {
    
    const float source_time = evaluate_source_time(curve, dest_time);
    
    // Calculate nearest integer frame boundaries in source time space
    const double frame_a = std::floor(static_cast<double>(source_time) / static_cast<double>(frame_duration)) * static_cast<double>(frame_duration);
    const double frame_b = frame_a + static_cast<double>(frame_duration);
    
    const float factor = (frame_duration > 1e-6f) 
        ? static_cast<float>((static_cast<double>(source_time) - frame_a) / static_cast<double>(frame_duration))
        : 0.0f;
    
    return { frame_a, frame_b, std::clamp(factor, 0.0f, 1.0f) };
}

} // namespace tachyon::timeline
