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

} // namespace

void Animation3DPresetRegistry::load_builtins() {
    // 1. Orbit Y: Rotate around Y axis
    register_spec({
        "tachyon.anim3d.orbit_y",
        {"tachyon.anim3d.orbit_y", "Orbit Y", "Continuous rotation around Y axis.", "animation.3d", {"orbit", "y-axis", "loop"}},
        [](LayerSpec& layer, const Animation3DParams& params) {
            auto& rot_y = layer.transform.three_d.rotation_y_property;
            rot_y.keyframes.clear();
            add_scalar_kf(rot_y.keyframes, params.delay, 0.0);
            add_scalar_kf(rot_y.keyframes, params.delay + params.duration, 360.0 * params.intensity);
            rot_y.post_infinity = "cycle";
        }
    });

    // 2. Float Z: Subtle oscillation in depth
    register_spec({
        "tachyon.anim3d.float_z",
        {"tachyon.anim3d.float_z", "Float Z", "Slow depth oscillation.", "animation.3d", {"float", "z-axis", "subtle"}},
        [](LayerSpec& layer, const Animation3DParams& params) {
            auto& pos_z = layer.transform.three_d.position_z_property;
            pos_z.keyframes.clear();
            double offset = 50.0 * params.intensity;
            add_scalar_kf(pos_z.keyframes, params.delay, 0.0, animation::EasingPreset::EaseInOut);
            add_scalar_kf(pos_z.keyframes, params.delay + params.duration * 0.5, offset, animation::EasingPreset::EaseInOut);
            add_scalar_kf(pos_z.keyframes, params.delay + params.duration, 0.0, animation::EasingPreset::EaseInOut);
            pos_z.post_infinity = "cycle";
        }
    });

    // 3. Turntable: Slow continuous rotation
    register_spec({
        "tachyon.anim3d.turntable",
        {"tachyon.anim3d.turntable", "Turntable", "Slow steady rotation like a turntable.", "animation.3d", {"rotate", "loop", "steady"}},
        [](LayerSpec& layer, const Animation3DParams& params) {
            auto& rot_y = layer.transform.three_d.rotation_y_property;
            rot_y.keyframes.clear();
            add_scalar_kf(rot_y.keyframes, params.delay, 0.0);
            add_scalar_kf(rot_y.keyframes, params.delay + params.duration, 360.0 * params.intensity);
            rot_y.post_infinity = "cycle";
        }
    });

    // 4. Camera Push In: Zooming into the scene
    register_spec({
        "tachyon.anim3d.camera_push_in",
        {"tachyon.anim3d.camera_push_in", "Camera Push In", "Zooming into the 3D scene.", "animation.3d", {"camera", "zoom", "entrance"}},
        [](LayerSpec& layer, const Animation3DParams& params) {
            auto& pos_z = layer.transform.three_d.position_z_property;
            pos_z.keyframes.clear();
            add_scalar_kf(pos_z.keyframes, params.delay, -200.0 * params.intensity, animation::EasingPreset::EaseOut);
            add_scalar_kf(pos_z.keyframes, params.delay + params.duration, 0.0, animation::EasingPreset::EaseOut);
        }
    });
}

} // namespace tachyon::presets
