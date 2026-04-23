#include "tachyon/renderer3d/effects/depth_of_field.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace tachyon::renderer3d {

namespace {
// Magic number constants to eliminate hardcoded values
constexpr float SENSOR_COVERAGE_FOR_MAX_BLUR = 0.05f;  // 5% of sensor width = max blur radius
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = 0.5f * PI;

// Luminance weights (BT.709)
constexpr float LUMINANCE_WEIGHT_R = 0.2126f;
constexpr float LUMINANCE_WEIGHT_G = 0.7152f;
constexpr float LUMINANCE_WEIGHT_B = 0.0722f;

// Depth-aware weighting constants
constexpr float DEPTH_FALLOFF_SHARPNESS = 4.0f;
constexpr float FOREGROUND_DEPTH_THRESHOLD = 0.1f;
constexpr float BACKGROUND_WEIGHT = 0.1f;
constexpr float FOREGROUND_WEIGHT = 1.0f;

// Highlight boost
constexpr float HIGHLIGHT_BOOST_FACTOR = 2.0f;
} // namespace


float DepthOfFieldPostPass::compute_coc_radius(
    float depth,
    float focal_distance,
    float aperture_fstop,
    float focal_length_mm,
    float sensor_width_mm) const 
{
    if (depth <= 0.0f || focal_distance <= 0.0f || aperture_fstop <= 0.0f) return 0.0f;

    // Circle of Confusion (CoC) diameter in mm:
    // CoC = |depth - focal_distance| / depth * (focal_length^2 / (aperture_fstop * (focal_distance - focal_length)))
    // Simplified: CoC ~ abs(depth - focal_distance) / depth * (focal_length / aperture_fstop) * scale_factor
    
    float coc_mm = std::abs(depth - focal_distance) / depth * (focal_length_mm / aperture_fstop);
    
    // Convert to pixels: assume sensor_width maps to image width (approx)
    // We use a relative scale: max_blur_radius_px at some reference CoC
    float scale = config_.max_blur_radius_px / (sensor_width_mm * SENSOR_COVERAGE_FOR_MAX_BLUR);
    return std::min(coc_mm * scale, config_.max_blur_radius_px);
}

void DepthOfFieldPostPass::separable_blur(
    std::vector<float>& rgba,
    const std::vector<float>& depth,
    std::uint32_t width,
    std::uint32_t height,
    float max_radius) const 
{
    (void)max_radius;
    if (width == 0 || height == 0 || rgba.empty()) return;

    std::vector<float> temp(rgba.size(), 0.0f);
    const int samples = std::max(4, config_.blur_samples);
    const float inv_samples = 1.0f / static_cast<float>(samples);

    // Horizontal pass
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            std::size_t idx = y * width + x;
            float d = depth[idx];
            float radius = compute_coc_radius(d, config_.focal_distance, config_.aperture_fstop,
                                               config_.focal_length_mm, config_.sensor_width_mm);
            if (radius < 0.5f) {
                // In focus: copy as-is
                for (int c = 0; c < 4; ++c) temp[idx * 4 + c] = rgba[idx * 4 + c];
                continue;
            }

            float r_sum = 0.0f, g_sum = 0.0f, b_sum = 0.0f, a_sum = 0.0f, w_sum = 0.0f;
            for (int s = -samples; s <= samples; ++s) {
                int sx = static_cast<int>(x) + static_cast<int>(static_cast<float>(s) * radius * inv_samples);
                sx = std::clamp(sx, 0, static_cast<int>(width) - 1);
                std::size_t sidx = y * width + static_cast<std::uint32_t>(sx);
                
                // Depth-aware weighting: reduce bleeding from far different depths
                float sd = depth[sidx];
                float depth_diff = std::abs(sd - d) / std::max(d, 1.0f);
                float weight = std::exp(-depth_diff * DEPTH_FALLOFF_SHARPNESS);
                
                r_sum += rgba[sidx * 4 + 0] * weight;
                g_sum += rgba[sidx * 4 + 1] * weight;
                b_sum += rgba[sidx * 4 + 2] * weight;
                a_sum += rgba[sidx * 4 + 3] * weight;
                w_sum += weight;
            }
            if (w_sum > 1e-6f) {
                temp[idx * 4 + 0] = r_sum / w_sum;
                temp[idx * 4 + 1] = g_sum / w_sum;
                temp[idx * 4 + 2] = b_sum / w_sum;
                temp[idx * 4 + 3] = a_sum / w_sum;
            } else {
                for (int c = 0; c < 4; ++c) temp[idx * 4 + c] = rgba[idx * 4 + c];
            }
        }
    }

    // Vertical pass
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            std::size_t idx = y * width + x;
            float d = depth[idx];
            float radius = compute_coc_radius(d, config_.focal_distance, config_.aperture_fstop,
                                               config_.focal_length_mm, config_.sensor_width_mm);
            if (radius < 0.5f) {
                for (int c = 0; c < 4; ++c) rgba[idx * 4 + c] = temp[idx * 4 + c];
                continue;
            }

            float r_sum = 0.0f, g_sum = 0.0f, b_sum = 0.0f, a_sum = 0.0f, w_sum = 0.0f;
            for (int s = -samples; s <= samples; ++s) {
                int sy = static_cast<int>(y) + static_cast<int>(static_cast<float>(s) * radius * inv_samples);
                sy = std::clamp(sy, 0, static_cast<int>(height) - 1);
                std::size_t sidx = static_cast<std::uint32_t>(sy) * width + x;
                
                float sd = depth[sidx];
                float depth_diff = std::abs(sd - d) / std::max(d, 1.0f);
                float weight = std::exp(-depth_diff * DEPTH_FALLOFF_SHARPNESS);
                
                r_sum += temp[sidx * 4 + 0] * weight;
                g_sum += temp[sidx * 4 + 1] * weight;
                b_sum += temp[sidx * 4 + 2] * weight;
                a_sum += temp[sidx * 4 + 3] * weight;
                w_sum += weight;
            }
            if (w_sum > 1e-6f) {
                rgba[idx * 4 + 0] = r_sum / w_sum;
                rgba[idx * 4 + 1] = g_sum / w_sum;
                rgba[idx * 4 + 2] = b_sum / w_sum;
                rgba[idx * 4 + 3] = a_sum / w_sum;
            } else {
                for (int c = 0; c < 4; ++c) rgba[idx * 4 + c] = temp[idx * 4 + c];
            }
        }
    }
}

void DepthOfFieldPostPass::circular_convolution_pass(
    std::vector<float>& rgba,
    const std::vector<float>& depth,
    std::uint32_t width,
    std::uint32_t height) const 
{
    if (width == 0 || height == 0 || rgba.empty()) return;

    std::vector<float> output = rgba;
    const int samples = config_.blur_samples;
    
    // Precompute poisson-disk or circular samples
    std::vector<std::pair<float, float>> sample_offsets;
    for (int i = 0; i < samples; ++i) {
        float angle = TWO_PI * (float)i / (float)samples;
        float dist = std::sqrt((float)i / (float)samples); // uniform distribution in circle
        sample_offsets.push_back({std::cos(angle) * dist, std::sin(angle) * dist});
    }

    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            std::size_t idx = y * width + x;
            float d = depth[idx];
            float radius = compute_coc_radius(d, config_.focal_distance, config_.aperture_fstop,
                                               config_.focal_length_mm, config_.sensor_width_mm);
            
            if (radius < 0.5f) continue;

            float r_sum = 0.0f, g_sum = 0.0f, b_sum = 0.0f, a_sum = 0.0f, w_sum = 0.0f;
            
            // Central sample
            r_sum += rgba[idx * 4 + 0];
            g_sum += rgba[idx * 4 + 1];
            b_sum += rgba[idx * 4 + 2];
            a_sum += rgba[idx * 4 + 3];
            w_sum = 1.0f;

            for (const auto& offset : sample_offsets) {
                float sx = (float)x + offset.first * radius;
                float sy = (float)y + offset.second * radius;
                
                int ix = static_cast<int>(std::clamp(sx, 0.0f, static_cast<float>(width) - 1.0f));
                int iy = static_cast<int>(std::clamp(sy, 0.0f, static_cast<float>(height) - 1.0f));
                
                std::size_t sidx = static_cast<std::size_t>(iy) * width + static_cast<std::size_t>(ix);
                float sd = depth[sidx];
                float sr = compute_coc_radius(sd, config_.focal_distance, config_.aperture_fstop,
                                               config_.focal_length_mm, config_.sensor_width_mm);
                
                // Per-sample CoC-based depth-aware weighting:
                // A sample contributes fully if its CoC radius is >= its distance from center.
                // Foreground bleeds more than background to avoid halos.
                float sample_dist = std::sqrt(offset.first * offset.first + offset.second * offset.second) * radius;
                float weight = 1.0f;
                if (sr < sample_dist && sd < d) {
                    weight = BACKGROUND_WEIGHT;
                } else if (sr < sample_dist && sd > d) {
                    weight = FOREGROUND_WEIGHT;
                }
                
                r_sum += rgba[sidx * 4 + 0] * weight;
                g_sum += rgba[sidx * 4 + 1] * weight;
                b_sum += rgba[sidx * 4 + 2] * weight;
                a_sum += rgba[sidx * 4 + 3] * weight;
                w_sum += weight;
            }

            output[idx * 4 + 0] = r_sum / w_sum;
            output[idx * 4 + 1] = g_sum / w_sum;
            output[idx * 4 + 2] = b_sum / w_sum;
            output[idx * 4 + 3] = a_sum / w_sum;
        }
    }
    rgba = std::move(output);
}

void DepthOfFieldPostPass::cinematic_bokeh_pass(
    std::vector<float>& rgba,
    const std::vector<float>& depth,
    std::uint32_t width,
    std::uint32_t height) const 
{
    if (width == 0 || height == 0 || rgba.empty()) return;

    std::vector<float> output = rgba;
    const int samples = config_.blur_samples;
    const int blades = config_.diaphragm_blades;
    const float rotation = config_.bokeh_rotation_deg * PI / 180.0f;
    
    // Precompute polygon or circular samples
    std::vector<std::pair<float, float>> sample_offsets;
    for (int i = 0; i < samples; ++i) {
        float angle = TWO_PI * (float)i / (float)samples;
        float dist = std::sqrt((float)i / (float)samples);
        
        if (blades >= 3) {
            // Polygonal bokeh shape
            float segment_angle = TWO_PI / (float)blades;
            float a = std::fmod(angle, segment_angle);
            float r = std::cos(segment_angle / 2.0f) / std::cos(a - segment_angle / 2.0f);
            dist *= r;
        }
        
        float ca = std::cos(angle + rotation);
        float sa = std::sin(angle + rotation);
        sample_offsets.push_back({ca * dist, sa * dist});
    }

    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            std::size_t idx = y * width + x;
            float d = depth[idx];
            float radius = compute_coc_radius(d, config_.focal_distance, config_.aperture_fstop,
                                               config_.focal_length_mm, config_.sensor_width_mm);
            
            if (radius < 0.5f) continue;

            float r_sum = 0.0f, g_sum = 0.0f, b_sum = 0.0f, a_sum = 0.0f, w_sum = 0.0f;
            
            // Central sample
            r_sum += rgba[idx * 4 + 0];
            g_sum += rgba[idx * 4 + 1];
            b_sum += rgba[idx * 4 + 2];
            a_sum += rgba[idx * 4 + 3];
            w_sum = 1.0f;

            for (const auto& offset : sample_offsets) {
                float sx = (float)x + offset.first * radius;
                float sy = (float)y + offset.second * radius;
                
                int ix = static_cast<int>(std::clamp(sx, 0.0f, static_cast<float>(width) - 1.0f));
                int iy = static_cast<int>(std::clamp(sy, 0.0f, static_cast<float>(height) - 1.0f));
                
                std::size_t sidx = static_cast<std::size_t>(iy) * width + static_cast<std::size_t>(ix);
                float sd = depth[sidx];
                float sr = compute_coc_radius(sd, config_.focal_distance, config_.aperture_fstop,
                                               config_.focal_length_mm, config_.sensor_width_mm);
                
                // Per-sample CoC-based depth-aware weighting:
                float sample_dist = std::sqrt(offset.first * offset.first + offset.second * offset.second) * radius;
                float weight = 1.0f;
                if (sr < sample_dist && sd < d) {
                    weight = BACKGROUND_WEIGHT;
                } else if (sr < sample_dist && sd > d) {
                    weight = FOREGROUND_WEIGHT;
                }
                
                // Highlight boost for cinematic feel
                float current_lum = rgba[sidx * 4 + 0] * LUMINANCE_WEIGHT_R + rgba[sidx * 4 + 1] * LUMINANCE_WEIGHT_G + rgba[sidx * 4 + 2] * LUMINANCE_WEIGHT_B;
                if (current_lum > 1.0f) {
                    weight *= (1.0f + (current_lum - 1.0f) * HIGHLIGHT_BOOST_FACTOR);
                }
                
                r_sum += rgba[sidx * 4 + 0] * weight;
                g_sum += rgba[sidx * 4 + 1] * weight;
                b_sum += rgba[sidx * 4 + 2] * weight;
                a_sum += rgba[sidx * 4 + 3] * weight;
                w_sum += weight;
            }

            output[idx * 4 + 0] = r_sum / w_sum;
            output[idx * 4 + 1] = g_sum / w_sum;
            output[idx * 4 + 2] = b_sum / w_sum;
            output[idx * 4 + 3] = a_sum / w_sum;
        }
    }
    rgba = std::move(output);
}

void DepthOfFieldPostPass::apply(
    AOVBuffer& aovs,
    const math::Vector3& camera_position,
    const math::Vector3& camera_forward) const 
{
    (void)camera_position;
    (void)camera_forward;
    if (!config_.enabled || aovs.width == 0 || aovs.height == 0) return;

    if (config_.quality == BokehQualityTier::Draft) {
        separable_blur(aovs.beauty_rgba, aovs.depth_z, aovs.width, aovs.height, config_.max_blur_radius_px);
    } else if (config_.quality == BokehQualityTier::Preview) {
        circular_convolution_pass(aovs.beauty_rgba, aovs.depth_z, aovs.width, aovs.height);
    } else {
        cinematic_bokeh_pass(aovs.beauty_rgba, aovs.depth_z, aovs.width, aovs.height);
    }
}

} // namespace tachyon::renderer3d
