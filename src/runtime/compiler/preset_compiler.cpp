#include "tachyon/runtime/compiler/preset_compiler.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <fstream>
#include <sstream>

namespace tachyon {

bool PresetLibrary::load_from_directory(const std::filesystem::path& dir) {
    // JSON preset loading has been removed.
    // Presets should be defined via C++ Builder API.
    (void)dir;
    return false;
}

const AnimationPreset* PresetLibrary::find(const std::string& name) const {
    auto it = m_presets.find(name);
    if (it != m_presets.end()) {
        return &it->second;
    }
    return nullptr;
}

PresetCompiler::PresetCompiler(const PresetLibrary& library)
    : m_library(library) {
}

void PresetCompiler::expand(SceneSpec& scene) const {
    for (auto& comp : scene.compositions) {
        for (auto& layer : comp.layers) {
            expand_layer(layer);
        }
    }
}

PresetCompiler::ResolvedLayerAnimationPresets
PresetCompiler::resolve_layer_animation_presets(const LayerSpec& layer) const {
    ResolvedLayerAnimationPresets resolved;

    resolved.in_id = layer.transition_in.transition_id;
    resolved.in_duration = layer.transition_in.duration;
    resolved.out_id = layer.transition_out.transition_id;
    resolved.out_duration = layer.transition_out.duration;
    resolved.during_id = "";

    return resolved;
}

void PresetCompiler::expand_layer(LayerSpec& layer) const {
    const double in_point = layer.playback.timing.start;
    const double out_point = layer.playback.timing.start + layer.playback.timing.duration;

    const auto presets = resolve_layer_animation_presets(layer);

    // Resolve Animation In
    const std::string in_id = presets.in_id;
    if (!in_id.empty()) {
        const AnimationPreset* preset = m_library.find(in_id);
        if (preset) {
            const double duration = presets.in_duration;
            const double phase_end = in_point + duration;
            inject_phase(layer, *preset, in_point, phase_end);
        }
    }

    // Resolve Animation During
    const std::string during_id = presets.during_id;
    if (!during_id.empty()) {
        const AnimationPreset* preset = m_library.find(during_id);
        if (preset) {
            inject_phase(layer, *preset, in_point, out_point);
        }
    }

    // Resolve Animation Out
    const std::string out_id = presets.out_id;
    if (!out_id.empty()) {
        const AnimationPreset* preset = m_library.find(out_id);
        if (preset) {
            const double duration = presets.out_duration;
            const double phase_start = out_point - duration;
            inject_phase(layer, *preset, phase_start, out_point);
        }
    }
}

void PresetCompiler::inject_phase(LayerSpec& layer,
                                   const AnimationPreset& preset,
                                   double phase_start,
                                   double phase_end) const {
    const double phase_duration = phase_end - phase_start;
    if (phase_duration <= 0.0) return;

    for (const auto& kf : preset.keyframes) {
        const double abs_t = phase_start + kf.t * phase_duration;

        if (kf.scale_x.has_value() || kf.scale_y.has_value()) {
            Vector2KeyframeSpec skf;
            skf.time = abs_t;
            skf.value = layer.transform.transform.scale_property.value.value_or(math::Vector2{1.0f, 1.0f});
            if (kf.scale_x.has_value()) {
                skf.value.x = static_cast<float>(*kf.scale_x);
            }
            if (kf.scale_y.has_value()) {
                skf.value.y = static_cast<float>(*kf.scale_y);
            }
            layer.transform.transform.scale_property.keyframes.push_back(std::move(skf));
        }
        if (kf.position_x.has_value() || kf.position_y.has_value()) {
            Vector2KeyframeSpec skf;
            skf.time = abs_t;
            skf.value = layer.transform.transform.position_property.value.value_or(math::Vector2::zero());
            if (kf.position_x.has_value()) {
                skf.value.x = static_cast<float>(*kf.position_x);
            }
            if (kf.position_y.has_value()) {
                skf.value.y = static_cast<float>(*kf.position_y);
            }
            layer.transform.transform.position_property.keyframes.push_back(std::move(skf));
        }
        if (kf.opacity.has_value()) {
            ScalarKeyframeSpec skf;
            skf.time = abs_t;
            skf.value = *kf.opacity;
            layer.transform.opacity_property.keyframes.push_back(std::move(skf));
        }
        if (kf.rotation.has_value()) {
            ScalarKeyframeSpec skf;
            skf.time = abs_t;
            skf.value = *kf.rotation;
            layer.transform.transform.rotation_property.keyframes.push_back(std::move(skf));
        }
    }
}

} // namespace tachyon
