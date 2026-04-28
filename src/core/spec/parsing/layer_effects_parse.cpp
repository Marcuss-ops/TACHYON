#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <algorithm>

using json = nlohmann::json;

namespace tachyon {

void parse_effects(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    if (object.contains("effects") && object.at("effects").is_array()) {
        for (const auto& effect : object.at("effects")) {
            if (!effect.is_object()) continue;
            EffectSpec spec;
            read_string(effect, "type", spec.type);
            read_bool(effect, "enabled", spec.enabled);
            if (effect.contains("scalars") && effect.at("scalars").is_object()) {
                for (auto& [key, val] : effect.at("scalars").items()) {
                    if (val.is_number()) spec.scalars[key] = val.get<double>();
                }
            }
            if (effect.contains("colors") && effect.at("colors").is_object()) {
                for (auto& [key, val] : effect.at("colors").items()) {
                    if (val.is_object()) {
                        ColorSpec color;
                        read_number(val, "r", color.r);
                        read_number(val, "g", color.g);
                        read_number(val, "b", color.b);
                        read_number(val, "a", color.a);
                        spec.colors[key] = color;
                    }
                }
            }
            if (effect.contains("strings") && effect.at("strings").is_object()) {
                for (auto& [key, val] : effect.at("strings").items()) {
                    if (val.is_string()) spec.strings[key] = val.get<std::string>();
                }
            }
            layer.effects.push_back(std::move(spec));
        }
    }
}

void parse_text_animators(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    if (object.contains("text_animators") && object.at("text_animators").is_array()) {
        for (const auto& anim : object.at("text_animators")) {
            if (!anim.is_object()) {
                diagnostics.add_warning("layer.text_animators.invalid", "text_animator entries must be objects", path);
                continue;
            }
            try {
                layer.text_animators.push_back(anim.get<TextAnimatorSpec>());
            } catch (const std::exception& e) {
                diagnostics.add_warning("layer.text_animators.parse_failed", e.what(), path);
            }
        }
    }
}

void parse_text_highlights(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    if (object.contains("text_highlights") && object.at("text_highlights").is_array()) {
        for (const auto& high : object.at("text_highlights")) {
            if (!high.is_object()) {
                diagnostics.add_warning("layer.text_highlights.invalid", "text_highlight entries must be objects", path);
                continue;
            }
            try {
                layer.text_highlights.push_back(high.get<TextHighlightSpec>());
            } catch (const std::exception& e) {
                diagnostics.add_warning("layer.text_highlights.parse_failed", e.what(), path);
            }
        }
    }
}

} // namespace tachyon
