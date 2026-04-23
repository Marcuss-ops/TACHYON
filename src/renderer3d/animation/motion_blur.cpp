#include "tachyon/renderer3d/effects/motion_blur.h"
#include "tachyon/core/math/transform3.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer3d {

namespace {
    constexpr float kRadToDeg = 180.0f / 3.14159265358979323846f;
    constexpr float kEpsilon = 1e-6f;
} // namespace

MotionBlurRenderer::MotionBlurRenderer() = default;
MotionBlurRenderer::MotionBlurRenderer(const MotionBlurConfig& config) : config_(config) {}

void MotionBlurRenderer::set_config(const MotionBlurConfig& config) {
    config_ = config;
}

void MotionBlurRenderer::set_quality_tier(const std::string& tier) {
    quality_tier_ = tier;
}

std::vector<double> MotionBlurRenderer::compute_subframe_times(double frame_time, double frame_duration) const {
    std::vector<double> times;
    if (!is_enabled()) {
        times.push_back(frame_time);
        return times;
    }

    const double shutter_duration = compute_shutter_duration(frame_duration);
    const double shutter_offset = (config_.shutter_phase / 360.0) * frame_duration;
    const double start_time = frame_time - shutter_duration * 0.5 + shutter_offset;

    if (config_.samples <= 1) {
        times.push_back(frame_time);
        return times;
    }

    for (int i = 0; i < config_.samples; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(config_.samples - 1);
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

        float normalized_t = static_cast<float>(1.0 + (t - frame_time) / frame_duration);

        SubFrameState s;
        s.time_seconds = t;
        s.weight = evaluate_weight(i, config_.samples);
        total_weight += s.weight;

        s.camera_matrix = interpolate_world_matrix(
            base_state.camera.previous_camera_matrix,
            base_state.camera.camera.transform.to_matrix(),
            normalized_t);

        s.camera_position = s.camera_matrix.transform_point({0.0f, 0.0f, 0.0f});
        s.camera_fov = base_state.camera.camera.fov_y_rad * kRadToDeg;
        s.focal_distance = base_state.camera.focus_distance;
        s.aperture = base_state.camera.aperture;

        s.object_states.reserve(base_state.layers.size());
        for (const auto& layer : base_state.layers) {
            SubFrameState::ObjectState os;
            os.object_id = static_cast<std::uint32_t>(std::hash<std::string>{}(layer.id));
            os.world_matrix = interpolate_world_matrix(
                layer.previous_world_matrix,
                layer.world_matrix,
                normalized_t);
            s.object_states.push_back(os);
        }

        states.push_back(std::move(s));
    }

    if (total_weight > kEpsilon) {
        for (auto& s : states) {
            s.weight /= total_weight;
        }
    }

    return states;
}

float MotionBlurRenderer::evaluate_weight(int sample_index, int total_samples) const {
    if (total_samples <= 1) return 1.0f;
    float t = static_cast<float>(sample_index) / static_cast<float>(total_samples - 1);
    return evaluate_temporal_weight(t, config_.curve);
}

float MotionBlurRenderer::evaluate_temporal_weight(double normalized_time, const std::string& curve) const {
    if (curve == "box") return 1.0f;
    if (curve == "linear" || curve == "triangle") return static_cast<float>(1.0 - std::abs(2.0 * normalized_time - 1.0));
    if (curve == "gaussian") return static_cast<float>(std::exp(-4.0 * std::pow(2.0 * normalized_time - 1.0, 2.0)));
    return 1.0f;
}

MotionBlurRenderer::VelocityResult MotionBlurRenderer::compute_object_velocity(
    const math::Matrix4x4& prev_world_matrix,
    const math::Matrix4x4& curr_world_matrix,
    double delta_time) {

    if (delta_time <= 0.0) {
        return VelocityResult{};
    }

    math::Transform3 prev_trs = prev_world_matrix.to_transform();
    math::Transform3 curr_trs = curr_world_matrix.to_transform();

    VelocityResult result;
    result.linear_velocity = (curr_trs.position - prev_trs.position) / static_cast<float>(delta_time);

    math::Quaternion delta_rot = curr_trs.rotation * prev_trs.rotation.conjugated();
    delta_rot = delta_rot.normalized();

    float angle = 2.0f * std::acos(std::clamp(delta_rot.w, -1.0f, 1.0f));
    if (std::abs(angle) > kEpsilon) {
        float s = std::sqrt(1.0f - delta_rot.w * delta_rot.w);
        math::Vector3 axis(delta_rot.x / s, delta_rot.y / s, delta_rot.z / s);
        result.angular_velocity = axis * (angle / static_cast<float>(delta_time));
    }

    result.center_of_motion = curr_trs.position;
    return result;
}

math::Matrix4x4 MotionBlurRenderer::interpolate_world_matrix(
    const math::Matrix4x4& a,
    const math::Matrix4x4& b,
    float t) {

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

float MotionBlurRenderer::sample_box(float t) const {
    (void)t;
    return 1.0f;
}

float MotionBlurRenderer::sample_uniform(float t) const {
    (void)t;
    return 1.0f;
}

float MotionBlurRenderer::sample_linear(float t) const {
    return 1.0f - std::abs(2.0f * t - 1.0f);
}

float MotionBlurRenderer::sample_cubic(float t) const {
    float x = std::abs(2.0f * t - 1.0f);
    return 1.0f - (x * x * (3.0f - 2.0f * x));
}

} // namespace tachyon::renderer3d
