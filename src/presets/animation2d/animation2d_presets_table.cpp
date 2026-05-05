#include "tachyon/presets/animation2d/animation2d_preset_registry.h"
#include "tachyon/core/animation/easing.h"

namespace tachyon::presets {

namespace {

void add_keyframe(std::vector<ScalarKeyframeSpec>& kfs, double time, double value, animation::EasingPreset easing = animation::EasingPreset::None) {
    ScalarKeyframeSpec kf;
    kf.time = time;
    kf.value = value;
    kf.easing = easing;
    kfs.push_back(kf);
}

void add_keyframe_v2(std::vector<Vector2KeyframeSpec>& kfs, double time, double x, double y, animation::EasingPreset easing = animation::EasingPreset::None) {
    Vector2KeyframeSpec kf;
    kf.time = time;
    kf.value = math::Vector2(static_cast<float>(x), static_cast<float>(y));
    kf.easing = easing;
    kfs.push_back(kf);
}

} // namespace

void Animation2DPresetRegistry::load_builtins() {
    // 1. Pop In: A bouncy scale-up entrance
    register_spec({
        "tachyon.anim2d.pop_in",
        {"tachyon.anim2d.pop_in", "Pop In", "Bouncy scale-up entrance animation.", "animation.2d", {"entrance", "pop", "bouncy"}},
        [](LayerSpec& layer, const Animation2DParams& params) {
            auto& scale = layer.transform.scale_property;
            scale.keyframes.clear();
            add_keyframe_v2(scale.keyframes, params.delay, 0.0, 0.0, animation::EasingPreset::EaseOut);
            add_keyframe_v2(scale.keyframes, params.delay + params.duration * 0.7, 1.1 * params.intensity, 1.1 * params.intensity, animation::EasingPreset::EaseInOut);
            add_keyframe_v2(scale.keyframes, params.delay + params.duration, 1.0, 1.0, animation::EasingPreset::EaseOut);
        }
    });

    // 2. Soft Pulse: Gentle looping scale oscillation
    register_spec({
        "tachyon.anim2d.soft_pulse",
        {"tachyon.anim2d.soft_pulse", "Soft Pulse", "Gentle breathing scale animation.", "animation.2d", {"loop", "pulse", "subtle"}},
        [](LayerSpec& layer, const Animation2DParams& params) {
            auto& scale = layer.transform.scale_property;
            scale.keyframes.clear();
            double t = params.delay;
            double step = params.duration / 2.0;
            add_keyframe_v2(scale.keyframes, t, 1.0, 1.0, animation::EasingPreset::EaseInOut);
            add_keyframe_v2(scale.keyframes, t + step, 1.05 * params.intensity, 1.05 * params.intensity, animation::EasingPreset::EaseInOut);
            add_keyframe_v2(scale.keyframes, t + params.duration, 1.0, 1.0, animation::EasingPreset::EaseInOut);
        }
    });

    // 3. Subtle Drift: Slow floating motion in Y axis
    register_spec({
        "tachyon.anim2d.subtle_drift",
        {"tachyon.anim2d.subtle_drift", "Subtle Drift", "Slow floating vertical motion.", "animation.2d", {"loop", "drift", "subtle"}},
        [](LayerSpec& layer, const Animation2DParams& params) {
            auto& pos = layer.transform.position_property;
            pos.keyframes.clear();
            double offset = 20.0 * params.intensity;
            add_keyframe_v2(pos.keyframes, params.delay, 0.0, 0.0, animation::EasingPreset::EaseInOut);
            add_keyframe_v2(pos.keyframes, params.delay + params.duration * 0.5, 0.0, -offset, animation::EasingPreset::EaseInOut);
            add_keyframe_v2(pos.keyframes, params.delay + params.duration, 0.0, 0.0, animation::EasingPreset::EaseInOut);
        }
    });

    // 4. Smooth Rotation: Gentle back and forth rotation
    register_spec({
        "tachyon.anim2d.smooth_rotate",
        {"tachyon.anim2d.smooth_rotate", "Smooth Rotate", "Gentle pendulum-like rotation.", "animation.2d", {"loop", "rotate", "subtle"}},
        [](LayerSpec& layer, const Animation2DParams& params) {
            auto& rot = layer.transform.rotation_property;
            rot.keyframes.clear();
            double angle = 5.0 * params.intensity;
            add_keyframe(rot.keyframes, params.delay, 0.0, animation::EasingPreset::EaseInOut);
            add_keyframe(rot.keyframes, params.delay + params.duration * 0.25, angle, animation::EasingPreset::EaseInOut);
            add_keyframe(rot.keyframes, params.delay + params.duration * 0.75, -angle, animation::EasingPreset::EaseInOut);
            add_keyframe(rot.keyframes, params.delay + params.duration, 0.0, animation::EasingPreset::EaseInOut);
        }
    });
}

} // namespace tachyon::presets
