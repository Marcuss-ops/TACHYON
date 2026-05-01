#include "tachyon/roto/roto_brush.h"
#include "tachyon/ai/segmentation_provider.h"
#include "tachyon/tracker/optical_flow.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace tachyon::ai {

// ---------------------------------------------------------------------------
// RotoBrush Implementation
// ---------------------------------------------------------------------------

RotoBrush::RotoBrush(std::unique_ptr<SegmentationProvider> provider)
    : m_provider(std::move(provider))
    , m_config(Config{})
    , m_flow_calculator(tracker::OpticalFlowCalculator::Config{}) {}

bool RotoBrush::has_provider() const {
    return m_provider && m_provider->available();
}

std::string RotoBrush::provider_name() const {
    return m_provider ? m_provider->name() : "none";
}

SegmentationMask RotoBrush::generate_matte(
    const tracker::GrayImage& frame,
    const std::vector<SegmentationPrompt>& prompts) {
    
    if (m_provider && m_provider->available()) {
        return m_provider->segment_frame(frame, prompts);
    }
    
    // Fallback: return empty mask (user must do manual roto)
    SegmentationMask empty;
    empty.width = frame.width;
    empty.height = frame.height;
    empty.alpha.resize(frame.width * frame.height, 0.0f);
    return empty;
}

SegmentationMask RotoBrush::warp_mask_with_flow(
    const SegmentationMask& prev_mask,
    const tracker::OpticalFlowResult& flow,
    int width, int height) {
    
    SegmentationMask result;
    result.width = static_cast<uint32_t>(width);
    result.height = static_cast<uint32_t>(height);
    result.alpha.resize(width * height, 0.0f);
    
    // Warp each pixel using optical flow vectors
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Get flow vector at this position
            const auto* flow_vec = flow.get_flow_at_pixel(x, y);
            if (!flow_vec || !flow_vec->valid) {
                // No valid flow: sample from same position in previous mask
                if (x >= 0 && x < (int)prev_mask.width && 
                    y >= 0 && y < (int)prev_mask.height) {
                    result.alpha[y * width + x] = prev_mask.at(x, y);
                }
                continue;
            }
            
            // Apply consumer logic to decide how to use the flow
            tracker::OpticalFlowConsumer consumer;
            auto consumer_result = consumer.consume(*flow_vec);
            
            math::Vector2 src_pos;
            switch (consumer_result.action) {
                case tracker::OpticalFlowConsumer::Action::kFlow:
                    src_pos.x = static_cast<float>(x) - flow_vec->flow.x;
                    src_pos.y = static_cast<float>(y) - flow_vec->flow.y;
                    break;
                case tracker::OpticalFlowConsumer::Action::kBlend:
                    // Blend between flowed and stationary
                    src_pos.x = static_cast<float>(x) - flow_vec->flow.x * consumer_result.weight;
                    src_pos.y = static_cast<float>(y) - flow_vec->flow.y * consumer_result.weight;
                    break;
                case tracker::OpticalFlowConsumer::Action::kHold:
                case tracker::OpticalFlowConsumer::Action::kZeroMotion:
                default:
                    // No motion: sample from same position
                    src_pos.x = static_cast<float>(x);
                    src_pos.y = static_cast<float>(y);
                    break;
            }
            
            // Bilinear interpolation from source position
            int src_x = static_cast<int>(std::floor(src_pos.x));
            int src_y = static_cast<int>(std::floor(src_pos.y));
            float fx = src_pos.x - src_x;
            float fy = src_pos.y - src_y;
            
            float value = 0.0f;
            for (int dy = 0; dy <= 1; ++dy) {
                for (int dx = 0; dx <= 1; ++dx) {
                    int sx = src_x + dx;
                    int sy = src_y + dy;
                    if (sx >= 0 && sx < (int)prev_mask.width &&
                        sy >= 0 && sy < (int)prev_mask.height) {
                        float w = (dx == 0 ? 1.0f - fx : fx) * (dy == 0 ? 1.0f - fy : fy);
                        value += prev_mask.at(sx, sy) * w;
                    }
                }
            }
            
            result.alpha[y * width + x] = value;
        }
    }
    
    return result;
}

SegmentationMask RotoBrush::propagate(
    const tracker::GrayImage& prev_frame,
    const tracker::GrayImage& next_frame,
    const SegmentationMask& prev_mask,
    const tracker::OpticalFlowResult* flow_result) {
    
    // If AI provider is available, use it for propagation
    if (m_provider && m_provider->available()) {
        return m_provider->propagate_mask(prev_frame, next_frame, prev_mask);
    }
    
    // Fallback: use optical flow-based propagation
    if (!m_config.use_optical_flow) {
        // Hold frame fallback
        return prev_mask;
    }
    
    // Compute optical flow between frames
    tracker::OpticalFlowResult flow;
    if (flow_result) {
        flow = *flow_result;
    } else {
        // Convert GrayImage (float data view) to contiguous vector for OpticalFlowCalculator
        std::vector<float> prev_frame_data(prev_frame.data, prev_frame.data + prev_frame.width * prev_frame.height);
        std::vector<float> next_frame_data(next_frame.data, next_frame.data + next_frame.width * next_frame.height);
        flow = m_flow_calculator.compute(
            prev_frame_data,
            next_frame_data,
            prev_frame.width,
            prev_frame.height,
            1); // GrayImage is single-channel
    }
    
    // Check if we have enough confidence to propagate
    float avg_confidence = flow.average_confidence();
    if (avg_confidence < m_config.propagation_threshold) {
        // Low confidence: hold previous frame
        return prev_mask;
    }
    
    // Warp mask using optical flow
    return warp_mask_with_flow(prev_mask, flow, next_frame.width, next_frame.height);
}

renderer2d::MaskPath RotoBrush::matte_to_path(
    const SegmentationMask& mask,
    float simplify_threshold) {
    
    renderer2d::MaskPath path;
    path.is_closed = true;
    path.is_inverted = false;
    
    if (mask.alpha.empty() || mask.width == 0 || mask.height == 0) {
        return path;
    }
    
    // Simple contour extraction using marching squares
    // This is a basic implementation - production would use more sophisticated algorithms
    const float threshold = 0.5f;
    std::vector<math::Vector2> contour_points;
    
    // Find boundary pixels
    for (uint32_t y = 0; y < mask.height - 1; ++y) {
        for (uint32_t x = 0; x < mask.width - 1; ++x) {
            float tl = mask.at(x, y);
            float tr = mask.at(x + 1, y);
            float bl = mask.at(x, y + 1);
            float br = mask.at(x + 1, y + 1);
            
            // Check if this cell crosses the threshold
            bool top = tl >= threshold;
            bool right = tr >= threshold;
            bool bottom = bl >= threshold;
            bool left = br >= threshold;
            
            if (top != right || right != bottom || bottom != left) {
                // Add center point as approximation
                contour_points.push_back(math::Vector2(
                    static_cast<float>(x) + 0.5f,
                    static_cast<float>(y) + 0.5f));
            }
        }
    }
    
    if (contour_points.empty()) {
        return path;
    }
    
    // Simplify contour using Douglas-Peucker algorithm
    auto douglas_peucker = [&](auto&& self, const std::vector<math::Vector2>& points,
                               int start, int end, float epsilon) -> std::vector<math::Vector2> {
        if (end <= start + 1) {
            return {points[start]};
        }
        
    // Find point with maximum distance from line
    float max_dist = 0;
    int max_idx = start;
    math::Vector2 p1 = points[start];
    math::Vector2 p2 = points[end];
    auto distance_to_line = [](const math::Vector2& point, const math::Vector2& line_start, const math::Vector2& line_end) {
        const float dx = line_end.x - line_start.x;
        const float dy = line_end.y - line_start.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        if (length <= 1e-6f) {
            const float px = point.x - line_start.x;
            const float py = point.y - line_start.y;
            return std::sqrt(px * px + py * py);
        }

        const float px = point.x - line_start.x;
        const float py = point.y - line_start.y;
        return std::abs(dx * py - dy * px) / length;
    };
    
    for (int i = start + 1; i < end; ++i) {
            float dist = distance_to_line(points[i], p1, p2);
            if (dist > max_dist) {
                max_dist = dist;
                max_idx = i;
            }
        }
        
        std::vector<math::Vector2> result;
        if (max_dist > epsilon) {
            auto left = self(self, points, start, max_idx, epsilon);
            auto right = self(self, points, max_idx, end, epsilon);
            result.insert(result.end(), left.begin(), left.end());
            result.insert(result.end(), right.begin(), right.end());
        } else {
            result.push_back(p1);
            result.push_back(p2);
        }
        return result;
    };
    
    auto simplified = douglas_peucker(douglas_peucker, contour_points, 0, 
                                      static_cast<int>(contour_points.size()) - 1, 
                                      simplify_threshold);
    
    // Convert to bezier vertices
    for (size_t i = 0; i < simplified.size(); ++i) {
        renderer2d::MaskVertex vertex;
        vertex.position = simplified[i];
        vertex.in_tangent = math::Vector2(0, 0);
        vertex.out_tangent = math::Vector2(0, 0);
        vertex.feather_inner = 2.0f; // Default feather
        vertex.feather_outer = 2.0f;
        path.vertices.push_back(vertex);
    }
    
    return path;
}

std::vector<SegmentationMask> RotoBrush::generate_sequence(
    const std::vector<tracker::GrayImage>& frames,
    int start_frame,
    const std::vector<SegmentationPrompt>& initial_prompts) {
    
    std::vector<SegmentationMask> result;
    
    if (frames.empty()) {
        return result;
    }
    
    // Segment first frame
    SegmentationMask current_mask = generate_matte(frames[0], initial_prompts);
    if (current_mask.alpha.empty()) {
        return result; // Failed to segment
    }
    
    result.push_back(current_mask);
    
    // Propagate to subsequent frames
    int propagated_count = 0;
    for (size_t i = 1; i < frames.size(); ++i) {
        // Re-segment if we've propagated too far
        if (propagated_count >= m_config.max_propagation_distance && has_provider()) {
            current_mask = generate_matte(frames[i], initial_prompts);
            propagated_count = 0;
        } else {
            current_mask = propagate(frames[i - 1], frames[i], current_mask);
            ++propagated_count;
        }
        
        if (current_mask.alpha.empty()) {
            break; // Propagation failed
        }
        
        result.push_back(current_mask);
    }
    
    return result;
}

} // namespace tachyon::ai
