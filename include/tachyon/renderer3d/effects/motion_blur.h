#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/matrix4x4.h"

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace tachyon::renderer3d {

class MotionBlurRenderer {
public:
    struct SubFrameState {
        double time_seconds{0.0};
        math::Vector3 camera_position;
        math::Vector3 camera_target;
        math::Vector3 camera_up{0.0f, 1.0f, 0.0f};
        math::Matrix4x4 camera_matrix;
        float camera_fov{60.0f};
        float focal_distance{100.0f};
        float aperture{0.0f};
        float weight{1.0f};

        // NEW: Interpolated matrices for all objects in the scene
        struct ObjectState {
            std::uint32_t object_id;
            math::Matrix4x4 world_matrix;
        };
        std::vector<ObjectState> object_states;
    };

    struct MotionBlurConfig {
        bool enabled{false};
        int samples{8};
        double shutter_angle{180.0};
        double shutter_phase{-90.0};
        std::string curve{"box"};
    };

    MotionBlurRenderer();
    explicit MotionBlurRenderer(const MotionBlurConfig& config);

    void set_config(const MotionBlurConfig& config);
    const MotionBlurConfig& get_config() const { return config_; }

    bool is_enabled() const { return config_.enabled && config_.samples > 1; }
    int get_subframe_count() const { return is_enabled() ? config_.samples : 1; }

    std::vector<double> compute_subframe_times(double frame_time, double frame_duration) const;
    std::vector<SubFrameState> generate_subframe_states(
        const scene::EvaluatedCompositionState& base_state,
        double frame_time,
        double frame_duration) const;

    float evaluate_weight(int sample_index, int total_samples) const;
    float evaluate_temporal_weight(double normalized_time, const std::string& curve) const;

    struct VelocityResult {
        math::Vector3 linear_velocity;
        math::Vector3 angular_velocity;
        math::Vector3 center_of_motion;
    };

    static VelocityResult compute_object_velocity(
        const math::Matrix4x4& prev_world_matrix,
        const math::Matrix4x4& curr_world_matrix,
        double delta_time);

    static math::Matrix4x4 interpolate_world_matrix(
        const math::Matrix4x4& a,
        const math::Matrix4x4& b,
        float t);

    void set_quality_tier(const std::string& tier);
    std::string get_quality_tier() const { return quality_tier_; }

    struct ProfileData {
        double total_time_ms{0.0};
        double subframe_build_time_ms{0.0};
        double trace_time_ms{0.0};
        double compose_time_ms{0.0};
        int rendered_subframes{0};
        int culled_objects{0};
    };

    const ProfileData& get_profile() const { return profile_; }
    void reset_profile() { profile_ = {}; }

private:
    MotionBlurConfig config_;
    std::string quality_tier_;
    ProfileData profile_;

    double compute_shutter_duration(double frame_duration) const;
    float sample_box(float t) const;
    float sample_uniform(float t) const;
    float sample_linear(float t) const;
    float sample_cubic(float t) const;
};

} // namespace tachyon::renderer3d
