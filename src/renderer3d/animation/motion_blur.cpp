#include "tachyon/renderer3d/effects/motion_blur.h"
#include "tachyon/core/math/transform3.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <sstream>

namespace tachyon::renderer3d {

MotionBlurRenderer::MotionBlurRenderer() = default;
MotionBlurRenderer::MotionBlurRenderer(const MotionBlurConfig& config) : config_(config) {}

void MotionBlurRenderer::set_config(const MotionBlurConfig& config) {
    config_ = config;
}

std::vector<double> MotionBlurRenderer::compute_subframe_times(double frame_time, double frame_duration) const {
    std::vector<double> times;
    if (!is_enabled()) {
        times.push_back(frame_time);
        return times;
    }

    const double shutter_duration = compute_shutter_duration(frame_duration);
    // Center the shutter around frame_time by subtracting half duration
    const double shutter_offset = (config_.shutter_phase / 360.0) * frame_duration;
    const double start_time = frame_time - shutter_duration * 0.5 + shutter_offset;
    
    if (config_.samples <= 1) {
        times.push_back(frame_time);
        return times;
    }

    // Optional stochastic jitter
    const bool use_jitter = true; // Could be moved to config
    std::mt19937 rng(1337); // Fixed seed for deterministic output, or use config_.seed
    std::uniform_real_distribution<double> dist(-0.5, 0.5);

    for (int i = 0; i < config_.samples; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(config_.samples - 1);
        if (use_jitter && i > 0 && i < config_.samples - 1) {
             double jitter_amount = 0.5 / static_cast<double>(config_.samples - 1);
             t += dist(rng) * jitter_amount;
        }
        times.push_back(start_time + t * shutter_duration);
    }
    return times;
}

std::vector<MotionBlurRenderer::SubFrameState> MotionBlurRenderer::generate_subframe_states(
    const scene::EvaluatedCompositionState& base_state,
    double frame_time,
    double frame_duration) const {
    
    auto times = compute_subframe_times(frame_time, frame_duration);
    std::vector<SubFrameState> states;
    states.reserve(times.size());

    float total_weight = 0.0f;
    for (int i = 0; i < static_cast<int>(times.size()); ++i) {
        double t = times[i];
        
        // Normalized time in [0, 1] between previous frame (0.0) and current frame (1.0)
        float normalized_t = static_cast<float>(1.0 + (t - frame_time) / frame_duration);

        SubFrameState s;
        s.time_seconds = t;
        s.weight = evaluate_weight(i, config_.samples);
        total_weight += s.weight;
        
        // Interpolate camera
        s.camera_matrix = interpolate_world_matrix(
            base_state.camera.previous_camera_matrix,
            base_state.camera.camera.transform.to_matrix(),
            normalized_t);
            
        s.camera_position = s.camera_matrix.transform_point({0.0f, 0.0f, 0.0f});
        s.camera_fov = base_state.camera.camera.fov_y_rad * 180.0f / 3.14159f;
        s.focal_distance = base_state.camera.focus_distance;
        s.aperture = base_state.camera.aperture;

        // Interpolate layers/objects
        s.object_states.reserve(base_state.layers.size());
        for (const auto& layer : base_state.layers) {
            SubFrameState::ObjectState os;
            os.object_id = static_cast<std::uint32_t>(std::hash<std::string>{}(layer.id)); // Use consistent hash if ID is string
            os.world_matrix = interpolate_world_matrix(
                layer.previous_world_matrix,
                layer.world_matrix,
                normalized_t);
            s.object_states.push_back(os);
        }
        
        states.push_back(std::move(s));
    }

    // Normalize weights
    if (total_weight > 1e-6f) {
        for (auto& s : states) {
            s.weight /= total_weight;
        }
    }

    return states;
}

float MotionBlurRenderer::evaluate_weight(int sample_index, int total_samples) const {
     if (total_samples <= 1) return 1.0f;
     float t = static_cast<float>(sample_index) / static_cast<float>(total_samples - 1);
     switch (config_.weight_curve) {
         case MotionBlurWeightCurve::kBox: return 1.0f;
         case MotionBlurWeightCurve::kTriangle: return 1.0f - std::abs(2.0f * t - 1.0f);
         case MotionBlurWeightCurve::kGaussian: return static_cast<float>(std::exp(-4.0 * std::pow(2.0 * t - 1.0, 2.0)));
         default: return 1.0f;
     }
 }

float MotionBlurRenderer::evaluate_temporal_weight(double normalized_time, const std::string& curve) const {
    if (curve == "box") return 1.0f;
    if (curve == "linear" || curve == "triangle") return static_cast<float>(1.0 - std::abs(2.0 * normalized_time - 1.0));
    if (curve == "gaussian") return static_cast<float>(std::exp(-4.0 * std::pow(2.0 * normalized_time - 1.0, 2.0)));
    return 1.0f;
}

math::Matrix4x4 MotionBlurRenderer::interpolate_world_matrix(
    const math::Matrix4x4& a,
    const math::Matrix4x4& b,
    float t) {
    
    // Matrix interpolation: Decompose to TRS, lerp/slerp, and recompose
    math::Transform3 trs_a = a.to_transform();
    math::Transform3 trs_b = b.to_transform();
    
    math::Vector3 pos = trs_a.position * (1.0f - t) + trs_b.position * t;
    math::Quaternion rot = math::Quaternion::slerp(trs_a.rotation, trs_b.rotation, t);
    math::Vector3 scale = trs_a.scale * (1.0f - t) + trs_b.scale * t;
    
    return math::compose_trs(pos, rot, scale);
}

double MotionBlurRenderer::compute_shutter_duration(double frame_duration) const {
     return (config_.shutter_angle / 360.0) * frame_duration;
 }

std::string MotionBlurRenderer::cache_identity() const {
    std::ostringstream oss;
    oss << "mb_samples:" << config_.samples
        << "|shutter_angle:" << config_.shutter_angle
        << "|shutter_phase:" << config_.shutter_phase
        << "|curve:" << static_cast<int>(config_.weight_curve)
        << "|cam_blur:" << config_.enable_camera_blur
        << "|obj_blur:" << config_.enable_object_blur;
    return oss.str();
}

} // namespace tachyon::renderer3d
