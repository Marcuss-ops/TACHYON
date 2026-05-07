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

    // Modern fields only
    resolved.in_id = layer.animation_in_preset;
    resolved.during_id = layer.animation_during_preset;
    resolved.out_id = layer.animation_out_preset;

    resolved.in_duration = static_cast<double>(layer.animation_in_duration);
    resolved.out_duration = static_cast<double>(layer.animation_out_duration);

    return resolved;
}

void PresetCompiler::expand_layer(LayerSpec& layer) const {
    // Handle duration -> out_point
    if (layer.duration.has_value()) {
        layer.out_point = layer.start_time + *layer.duration;
    }

    const double in_point = layer.in_point;
    const double out_point = layer.out_point;

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
            skf.value = layer.transform.scale_property.value.value_or(math::Vector2{1.0f, 1.0f});
            if (kf.scale_x.has_value()) {
                skf.value.x = static_cast<float>(*kf.scale_x);
            }
            if (kf.scale_y.has_value()) {
                skf.value.y = static_cast<float>(*kf.scale_y);
            }
            layer.transform.scale_property.keyframes.push_back(std::move(skf));
        }
        if (kf.position_x.has_value() || kf.position_y.has_value()) {
            Vector2KeyframeSpec skf;
            skf.time = abs_t;
            skf.value = layer.transform.position_property.value.value_or(math::Vector2::zero());
            if (kf.position_x.has_value()) {
                skf.value.x = static_cast<float>(*kf.position_x);
            }
            if (kf.position_y.has_value()) {
                skf.value.y = static_cast<float>(*kf.position_y);
            }
            layer.transform.position_property.keyframes.push_back(std::move(skf));
        }
        if (kf.opacity.has_value()) {
            ScalarKeyframeSpec skf;
            skf.time = abs_t;
            skf.value = *kf.opacity;
            layer.opacity_property.keyframes.push_back(std::move(skf));
        }
        if (kf.rotation.has_value()) {
            ScalarKeyframeSpec skf;
            skf.time = abs_t;
            skf.value = *kf.rotation;
            layer.transform.rotation_property.keyframes.push_back(std::move(skf));
        }
    }
}

} // namespace tachyon
