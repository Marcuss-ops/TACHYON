#include "tachyon/renderer3d/effects/motion_blur.h"
#include <algorithm>
#include <cmath>

namespace tachyon::renderer3d {

MotionBlurRenderer::MotionBlurRenderer() {
    config_.enabled = false;
    config_.samples = 8;
    config_.shutter_angle = 180.0;
    config_.shutter_phase = -90.0;
    config_.curve = "box";
    quality_tier_ = "high";
}

MotionBlurRenderer::MotionBlurRenderer(const MotionBlurConfig& config) : config_(config) {
    quality_tier_ = "high";
}

void MotionBlurRenderer::set_config(const MotionBlurConfig& config) {
    config_ = config;
    if (quality_tier_ == "medium" && config_.samples > 4) {
        config_.samples = 4;
    } else if (quality_tier_ == "low" && config_.samples > 2) {
        config_.samples = 2;
    }
}

std::vector<double> MotionBlurRenderer::compute_subframe_times(double frame_time, double frame_duration) const {
    std::vector<double> times;
    if (!is_enabled()) {
        times.push_back(frame_time);
        return times;
    }

    double shutter_duration = compute_shutter_duration(frame_duration);
    double shutter_start = frame_time + frame_duration * (config_.shutter_phase / 360.0);

    times.reserve(config_.samples);
    for (int i = 0; i < config_.samples; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(config_.samples - 1);
        double weight = evaluate_temporal_weight(t, config_.curve);
        double subframe_time = shutter_start + weight * shutter_duration;
        times.push_back(subframe_time);
    }

    return times;
}

std::vector<MotionBlurRenderer::SubFrameState> MotionBlurRenderer::generate_subframe_states(
    const scene::EvaluatedCompositionState& base_state,
    double frame_time,
    double frame_duration) const 
{
    std::vector<SubFrameState> states;
    if (!is_enabled() || !base_state.camera.available) {
        states.push_back({
            frame_time,
            base_state.camera.position,
            base_state.camera.camera.transform.to_matrix(),
            base_state.camera.camera.fov_y_rad
        });
        return states;
    }

    auto times = compute_subframe_times(frame_time, frame_duration);
    states.reserve(times.size());

    for (double t : times) {
        SubFrameState state;
        state.time_seconds = t;
        state.camera_position = base_state.camera.position;
        state.camera_matrix = base_state.camera.camera.transform.to_matrix();
        state.camera_fov = base_state.camera.camera.fov_y_rad;
        states.push_back(state);
    }

    return states;
}

float MotionBlurRenderer::evaluate_weight(int sample_index, int total_samples) const {
    if (total_samples <= 1) return 1.0f;
    float t = static_cast<float>(sample_index) / static_cast<float>(total_samples - 1);
    return evaluate_temporal_weight(t, config_.curve);
}

float MotionBlurRenderer::evaluate_temporal_weight(double normalized_time, const std::string& curve) const {
    if (curve == "box") return static_cast<float>(sample_box(static_cast<float>(normalized_time)));
    if (curve == "linear") return static_cast<float>(sample_linear(static_cast<float>(normalized_time)));
    if (curve == "cubic") return static_cast<float>(sample_cubic(static_cast<float>(normalized_time)));
    return static_cast<float>(sample_uniform(static_cast<float>(normalized_time)));
}

MotionBlurRenderer::VelocityResult MotionBlurRenderer::compute_object_velocity(
    const math::Matrix4x4& prev_world_matrix,
    const math::Matrix4x4& curr_world_matrix,
    double delta_time) 
{
    VelocityResult result;
    if (delta_time <= 0.0) return result;

    math::Vector3 prev_pos(prev_world_matrix[12], prev_world_matrix[13], prev_world_matrix[14]);
    math::Vector3 curr_pos(curr_world_matrix[12], curr_world_matrix[13], curr_world_matrix[14]);

    result.linear_velocity = (curr_pos - prev_pos) / static_cast<float>(delta_time);
    result.center_of_motion = (prev_pos + curr_pos) * 0.5f;

    return result;
}

math::Matrix4x4 MotionBlurRenderer::interpolate_world_matrix(
    const math::Matrix4x4& a,
    const math::Matrix4x4& b,
    float t) 
{
    math::Matrix4x4 result;
    for (int i = 0; i < 16; ++i) {
        result[i] = a[i] + (b[i] - a[i]) * t;
    }
    return result;
}

void MotionBlurRenderer::set_quality_tier(const std::string& tier) {
    quality_tier_ = tier;
    if (tier == "medium" && config_.samples > 4) {
        config_.samples = 4;
    } else if (tier == "low" && config_.samples > 2) {
        config_.samples = 2;
    } else if (tier == "high") {
        config_.samples = std::max(config_.samples, 8);
    }
}

double MotionBlurRenderer::compute_shutter_duration(double frame_duration) const {
    return frame_duration * (config_.shutter_angle / 360.0);
}

float MotionBlurRenderer::sample_box(float t) const {
    return t;
}

float MotionBlurRenderer::sample_uniform(float t) const {
    return t;
}

float MotionBlurRenderer::sample_linear(float t) const {
    return t * t * (3.0f - 2.0f * t);
}

float MotionBlurRenderer::sample_cubic(float t) const {
    float tt = t * t;
    return tt * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

} // namespace tachyon::renderer3d
