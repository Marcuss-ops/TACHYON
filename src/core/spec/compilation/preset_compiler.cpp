#include "tachyon/core/spec/compilation/preset_compiler.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tachyon {

bool PresetLibrary::load_from_directory(const std::filesystem::path& dir) {
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        return false;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const auto& path = entry.path();
        if (path.extension() != ".json") continue;

        std::ifstream file(path);
        if (!file) continue;

        std::stringstream buffer;
        buffer << file.rdbuf();
        try {
            const json parsed = json::parse(buffer.str());
            if (!parsed.is_object()) continue;

            AnimationPreset preset;
            if (parsed.contains("name") && parsed.at("name").is_string()) {
                preset.name = parsed.at("name").get<std::string>();
            } else {
                preset.name = path.stem().string();
            }

            if (parsed.contains("phase") && parsed.at("phase").is_string()) {
                preset.phase = parsed.at("phase").get<std::string>();
            }

            if (parsed.contains("keyframes") && parsed.at("keyframes").is_array()) {
                for (const auto& kf : parsed.at("keyframes")) {
                    if (!kf.is_object()) continue;
                    AnimationPreset::Keyframe keyframe;
                    read_number(kf, "t", keyframe.t);
                    if (kf.contains("scale_x") && kf.at("scale_x").is_number()) keyframe.scale_x = kf.at("scale_x").get<float>();
                    if (kf.contains("scale_y") && kf.at("scale_y").is_number()) keyframe.scale_y = kf.at("scale_y").get<float>();
                    if (kf.contains("position_x") && kf.at("position_x").is_number()) keyframe.position_x = kf.at("position_x").get<float>();
                    if (kf.contains("position_y") && kf.at("position_y").is_number()) keyframe.position_y = kf.at("position_y").get<float>();
                    if (kf.contains("opacity") && kf.at("opacity").is_number()) keyframe.opacity = kf.at("opacity").get<float>();
                    if (kf.contains("rotation") && kf.at("rotation").is_number()) keyframe.rotation = kf.at("rotation").get<float>();
                    if (kf.contains("blur_radius") && kf.at("blur_radius").is_number()) keyframe.blur_radius = kf.at("blur_radius").get<float>();
                    preset.keyframes.push_back(std::move(keyframe));
                }
            }

            m_presets[preset.name] = std::move(preset);
        } catch (const std::exception&) {
            continue;
        }
    }

    return !m_presets.empty();
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

void PresetCompiler::expand_layer(LayerSpec& layer) const {
    // Handle duration -> out_point
    if (layer.duration.has_value()) {
        layer.out_point = layer.start_time + *layer.duration;
    }

    const double in_point = layer.in_point;
    const double out_point = layer.out_point;

    // Expand "in" preset
    if (!layer.in_preset.empty()) {
        const AnimationPreset* preset = m_library.find(layer.in_preset);
        if (preset) {
            const double phase_end = in_point + layer.in_duration;
            inject_phase(layer, *preset, in_point, phase_end);
        }
    }

    // Expand "during" preset
    if (!layer.during_preset.empty()) {
        const AnimationPreset* preset = m_library.find(layer.during_preset);
        if (preset) {
            inject_phase(layer, *preset, in_point, out_point);
        }
    }

    // Expand "out" preset
    if (!layer.out_preset.empty()) {
        const AnimationPreset* preset = m_library.find(layer.out_preset);
        if (preset) {
            const double phase_start = out_point - layer.out_duration;
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
