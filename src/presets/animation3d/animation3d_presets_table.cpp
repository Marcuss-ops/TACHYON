#include "tachyon/presets/animation3d/animation3d_preset_registry.h"
#include "tachyon/core/animation/easing.h"

namespace tachyon::presets {

namespace {

void add_scalar_kf(std::vector<ScalarKeyframeSpec>& kfs, double time, double value, animation::EasingPreset easing = animation::EasingPreset::None) {
    ScalarKeyframeSpec kf;
    kf.time = time;
    kf.value = value;
    kf.easing = easing;
    kfs.push_back(kf);
}

void add_vector3_kf(std::vector<AnimatedVector3Spec::Keyframe>& kfs, double time, float x, float y, float z, animation::EasingPreset easing = animation::EasingPreset::None) {
    AnimatedVector3Spec::Keyframe kf;
    kf.time = time;
    kf.value = math::Vector3(x, y, z);
    kf.easing = easing;
    kfs.push_back(kf);
}

} // namespace

void Animation3DPresetRegistry::load_builtins() {
    using namespace registry;

    auto get_anim_params = [](const ParameterBag& p) {
        struct {
            double duration;
            double delay;
            float intensity;
        } res;
        res.duration  = p.get_or<double>("duration", 1.0);
        res.delay     = p.get_or<double>("delay", 0.0);
        res.intensity = static_cast<float>(p.get_or<double>("intensity", 1.0));
        return res;
    };

    // 1. Orbit Y: Rotate around Y axis
    register_spec({
        "tachyon.anim3d.orbit_y",
        {"tachyon.anim3d.orbit_y", "Orbit Y", "Continuous rotation around Y axis.", "animation.3d", {"orbit", "y-axis", "loop"}},
        {},
        [get_anim_params](LayerSpec& layer, const ParameterBag& p) {
            auto params = get_anim_params(p);
            auto& rot = layer.transform3d.rotation_property;
            rot.keyframes.clear();
            add_vector3_kf(rot.keyframes, params.delay, 0.0f, 0.0f, 0.0f);
            add_vector3_kf(rot.keyframes, params.delay + params.duration, 0.0f, 360.0f * params.intensity, 0.0f);
        }
    });

    // 2. Float Z: Subtle oscillation in depth
    register_spec({
        "tachyon.anim3d.float_z",
        {"tachyon.anim3d.float_z", "Float Z", "Slow depth oscillation.", "animation.3d", {"float", "z-axis", "subtle"}},
        {},
        [get_anim_params](LayerSpec& layer, const ParameterBag& p) {
            auto params = get_anim_params(p);
            auto& pos = layer.transform3d.position_property;
            pos.keyframes.clear();
            float offset = 50.0f * params.intensity;
            add_vector3_kf(pos.keyframes, params.delay, 0.0f, 0.0f, 0.0f, animation::EasingPreset::EaseInOut);
            add_vector3_kf(pos.keyframes, params.delay + params.duration * 0.5, 0.0f, 0.0f, offset, animation::EasingPreset::EaseInOut);
            add_vector3_kf(pos.keyframes, params.delay + params.duration, 0.0f, 0.0f, 0.0f, animation::EasingPreset::EaseInOut);
        }
    });

    // 3. Turntable: Slow continuous rotation
    register_spec({
        "tachyon.anim3d.turntable",
        {"tachyon.anim3d.turntable", "Turntable", "Slow steady rotation like a turntable.", "animation.3d", {"rotate", "loop", "steady"}},
        {},
        [get_anim_params](LayerSpec& layer, const ParameterBag& p) {
            auto params = get_anim_params(p);
            auto& rot = layer.transform3d.rotation_property;
            rot.keyframes.clear();
            add_vector3_kf(rot.keyframes, params.delay, 0.0f, 0.0f, 0.0f);
            add_vector3_kf(rot.keyframes, params.delay + params.duration, 0.0f, 360.0f * params.intensity, 0.0f);
        }
    });

    // 4. Camera Push In: Zooming into the scene
    register_spec({
        "tachyon.anim3d.camera_push_in",
        {"tachyon.anim3d.camera_push_in", "Camera Push In", "Zooming into the 3D scene.", "animation.3d", {"camera", "zoom", "entrance"}},
        {},
        [get_anim_params](LayerSpec& layer, const ParameterBag& p) {
            auto params = get_anim_params(p);
            auto& pos = layer.transform3d.position_property;
            pos.keyframes.clear();
            add_vector3_kf(pos.keyframes, params.delay, 0.0f, 0.0f, -200.0f * params.intensity, animation::EasingPreset::EaseOut);
            add_vector3_kf(pos.keyframes, params.delay + params.duration, 0.0f, 0.0f, 0.0f, animation::EasingPreset::EaseOut);
        }
    });
}

} // namespace tachyon::presets
