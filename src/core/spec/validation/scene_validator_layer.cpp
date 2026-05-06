#include "tachyon/core/spec/validation/scene_validator.h"
#include "tachyon/core/spec/validation/layer_spec_normalizer.h"
#include "tachyon/text/fonts/management/font_manifest.h"
#include <filesystem>
#include <fstream>

namespace tachyon::core {

namespace {

bool looks_like_media_reference(const std::string& value) {
    if (value.empty()) {
        return false;
    }

    const std::filesystem::path path(value);
    if (path.has_extension()) {
        return true;
    }

    return value.find('/') != std::string::npos || value.find('\\') != std::string::npos;
}

void warn_legacy_animation_preset(
    const std::string& legacy_value,
    const std::string& canonical_value,
    const std::string& path,
    const std::string& field_name,
    const std::string& replacement_name,
    ValidationResult& out) {
    if (legacy_value.empty()) {
        return;
    }

    const bool canonical_present = !canonical_value.empty();
    const std::string message = canonical_present
        ? field_name + " is legacy authoring data and is ignored because " + replacement_name + " is set."
        : field_name + " is legacy authoring data; use " + replacement_name + " instead.";

    out.issues.push_back(ValidationIssue{
        ValidationIssue::Severity::Warning,
        path + "." + field_name,
        message
    });
    out.warning_count++;
}

} // namespace

void SceneValidator::validate_layer(const ::tachyon::LayerSpec& layer, const ::tachyon::CompositionSpec& comp, const ::tachyon::SceneSpec& scene, const std::string& path, ValidationResult& out) const {
    const NormalizedLayerView normalized = normalize_layer_view(layer);

    if (layer.id.empty()) {
        out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, path + ".id", "Layer ID cannot be empty."});
        out.error_count++;
    }

    if (normalized.legacy_type_string_used) {
        out.issues.push_back(ValidationIssue{
            ValidationIssue::Severity::Warning,
            path + ".type_string",
            "type_string is legacy authoring data; use layer.type instead."
        });
        out.warning_count++;
    }

    if (normalized.type == LayerType::Unknown) {
        out.issues.push_back(ValidationIssue{
            ValidationIssue::Severity::Error,
            path + ".type",
            "Layer type is missing or unsupported."
        });
        out.error_count++;
    }

    warn_legacy_animation_preset(layer.in_preset, layer.animation_in_preset, path, "in_preset", "animation_in_preset", out);
    warn_legacy_animation_preset(layer.during_preset, layer.animation_during_preset, path, "during_preset", "animation_during_preset", out);
    warn_legacy_animation_preset(layer.out_preset, layer.animation_out_preset, path, "out_preset", "animation_out_preset", out);

    // Validate layer duration
    auto layer_duration = layer.duration.has_value() ? layer.duration.value() : (layer.out_point - layer.in_point);
    if (layer_duration <= 0.0) {
        out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, path + ".duration", "Layer duration must be greater than 0."});
        out.error_count++;
    }

    // Validate layer start time
    if (layer.start_time < 0.0f) {
        out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, path + ".start_time", "Layer start time cannot be negative."});
        out.error_count++;
    }

    // Check if layer extends beyond composition duration
    float layer_end = static_cast<float>(layer.start_time);
    if (layer.duration.has_value()) {
        layer_end += static_cast<float>(layer.duration.value());
    } else {
        layer_end += static_cast<float>(layer.out_point - layer.in_point);
    }
    if (layer_end > comp.duration + 0.001f) { // small epsilon for floating point comparison
        out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Warning, path + ".duration", 
            "Layer extends beyond composition duration (layer ends at " + std::to_string(layer_end) + 
            "s, composition is " + std::to_string(comp.duration) + "s)."});
        out.warning_count++;
    }

    if (layer.opacity < 0.0 || layer.opacity > 1.0) {
        out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, path + ".opacity", "Layer opacity must be between 0 and 1."});
        out.error_count++;
    }

    if (layer.width < 0 || layer.height < 0) {
        out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, path + ".dimensions", "Layer dimensions cannot be negative."});
        out.error_count++;
    }

    // Validate keyframes are within layer time range
    validate_keyframes(layer, path, out);

    // Validate track bindings
    validate_track_bindings(layer, path, out);
    
    // Validate safe area for text layers
    validate_safe_area(layer, comp, path, out);

    // Validate font references for text layers
    if (normalized.type == LayerType::Text) {
        validate_font_reference(layer, scene, path, out);
    }

    // Validate file references for image/video layers
    if (normalized.type == LayerType::Image || normalized.type == LayerType::Video) {
        validate_file_reference(layer, scene, path, out);
    }

    // Track matte validation: if a matte layer is specified, it must exist and must not be self
    if (layer.track_matte_layer_id.has_value() && !layer.track_matte_layer_id->empty()) {
        if (*layer.track_matte_layer_id == layer.id) {
            out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, path + ".track_matte_layer_id", "Track matte layer cannot reference itself."});
            out.error_count++;
        } else {
            bool found = false;
            for (const auto& l : comp.layers) {
                if (l.id == *layer.track_matte_layer_id) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, path + ".track_matte_layer_id", "References non-existent layer: " + *layer.track_matte_layer_id});
                out.error_count++;
            }
        }
        // Validate that track_matte_type is not None when a matte layer is specified
        if (layer.track_matte_type == TrackMatteType::None) {
            out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Warning, path + ".track_matte_type", "track_matte_layer_id is set but track_matte_type is None."});
        }
    }

    if (normalized.type == LayerType::Precomp) {
        if (!layer.precomp_id.has_value() || layer.precomp_id->empty()) {
            out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, path + ".precomp_id", "Precomp layer requires a composition reference."});
            out.error_count++;
        } else {
            bool found = false;
            for (const auto& c : scene.compositions) {
                if (c.id == *layer.precomp_id) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, path + ".precomp_id", "References non-existent composition: " + *layer.precomp_id});
                out.error_count++;
            }
        }
    }
}

void SceneValidator::validate_safe_area(const ::tachyon::LayerSpec& layer, const ::tachyon::CompositionSpec& comp, const std::string& path, ValidationResult& out) const {
    // Safe area YouTube = 10% margine su tutti i lati
    // Se layer ha testo E posizione fuori dall'area sicura → Warning
    
    // Applica solo a layer di tipo text
    if (canonical_layer_type(layer) != LayerType::Text) {
        return;
    }
    
    const float safe_x = comp.width * 0.10f;
    const float safe_y = comp.height * 0.10f;
    const float safe_width = comp.width - 2.0f * safe_x;
    const float safe_height = comp.height - 2.0f * safe_y;
    
    // Calcola bounding box del layer
    const float layer_left = static_cast<float>(layer.transform.position_x.value_or(0.0));
    const float layer_top = static_cast<float>(layer.transform.position_y.value_or(0.0));
    const float layer_right = layer_left + layer.width;
    const float layer_bottom = layer_top + layer.height;
    
    // Controlla se il layer esce dalla safe area
    bool outside_safe = false;
    std::string violation_details;
    
    if (layer_left < safe_x) {
        outside_safe = true;
        violation_details += "Left edge outside safe area. ";
    }
    if (layer_top < safe_y) {
        outside_safe = true;
        violation_details += "Top edge outside safe area. ";
    }
    if (layer_right > safe_x + safe_width) {
        outside_safe = true;
        violation_details += "Right edge outside safe area. ";
    }
    if (layer_bottom > safe_y + safe_height) {
        outside_safe = true;
        violation_details += "Bottom edge outside safe area. ";
    }
    
    if (outside_safe) {
        out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Warning, 
            path + ".transform.position",
            "Text layer may be outside YouTube safe area (10% margin). " + violation_details});
        out.warning_count++;
    }
}

void SceneValidator::validate_track_bindings(const ::tachyon::LayerSpec& layer, const std::string& path, ValidationResult& out) const {
    // Validate track bindings reference valid sources
    for (std::size_t i = 0; i < layer.track_bindings.size(); ++i) {
        const auto& binding = layer.track_bindings[i];
        
        if (binding.source_id.empty()) {
            out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error,
                path + ".track_bindings[" + std::to_string(i) + "].source_id",
                "Track binding source_id cannot be empty."});
            out.error_count++;
        }
        
        if (binding.source_track_name.empty()) {
            out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error,
                path + ".track_bindings[" + std::to_string(i) + "].source_track_name",
                "Track binding source_track_name cannot be empty."});
            out.error_count++;
        }
        
        if (binding.influence < 0.0f || binding.influence > 1.0f) {
            out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Warning,
                path + ".track_bindings[" + std::to_string(i) + "].influence",
                "Track binding influence should be between 0.0 and 1.0."});
            out.warning_count++;
        }
    }
}

void SceneValidator::validate_keyframes(const ::tachyon::LayerSpec& layer, const std::string& path, ValidationResult& out) const {
    // Validate that keyframe times are within the layer's time range
    float layer_start = static_cast<float>(layer.start_time);
    float layer_end = layer_start;
    if (layer.duration.has_value()) {
        layer_end += static_cast<float>(layer.duration.value());
    } else {
        layer_end += static_cast<float>(layer.out_point - layer.in_point);
    }
    
    // Check transform keyframes
    for (const auto& kf : layer.transform.position_property.keyframes) {
        if (kf.time < layer_start - 0.001f || kf.time > layer_end + 0.001f) {
            out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Warning, 
                path + ".transform.position_property.keyframes",
                "Keyframe at time " + std::to_string(kf.time) + "s is outside layer time range [" + 
                std::to_string(layer_start) + "s, " + std::to_string(layer_end) + "s]."});
            out.warning_count++;
        }
    }
    
    for (const auto& kf : layer.transform.scale_property.keyframes) {
        if (kf.time < layer_start - 0.001f || kf.time > layer_end + 0.001f) {
            out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Warning, 
                path + ".transform.scale_property.keyframes",
                "Keyframe at time " + std::to_string(kf.time) + "s is outside layer time range."});
            out.warning_count++;
        }
    }
    
    for (const auto& kf : layer.transform.rotation_property.keyframes) {
        if (kf.time < layer_start - 0.001f || kf.time > layer_end + 0.001f) {
            out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Warning, 
                path + ".transform.rotation_property.keyframes",
                "Keyframe at time " + std::to_string(kf.time) + "s is outside layer time range."});
            out.warning_count++;
        }
    }
    
    for (const auto& kf : layer.opacity_property.keyframes) {
        if (kf.time < layer_start - 0.001f || kf.time > layer_end + 0.001f) {
            out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Warning, 
                path + ".opacity_property.keyframes",
                "Keyframe at time " + std::to_string(kf.time) + "s is outside layer time range."});
            out.warning_count++;
        }
    }
}

void SceneValidator::validate_font_reference(const ::tachyon::LayerSpec& layer, const ::tachyon::SceneSpec& scene, const std::string& path, ValidationResult& out) const {
    if (!layer.font_id.empty() && scene.font_manifest) {
        bool font_found = false;
        for (const auto& font : scene.font_manifest->fonts) {
            if (font.id == layer.font_id) {
                font_found = true;
                break;
            }
        }
        if (!font_found) {
            out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, 
                path + ".font_id",
                "Font ID '" + layer.font_id + "' not found in font manifest."});
            out.error_count++;
        }
    }
}

void SceneValidator::validate_file_reference(const ::tachyon::LayerSpec& layer, const ::tachyon::SceneSpec& scene, const std::string& path, ValidationResult& out) const {
    const NormalizedLayerView normalized = normalize_layer_view(layer);
    if (normalized.type != LayerType::Image && normalized.type != LayerType::Video) {
        return;
    }

    if (normalized.asset_reference.empty()) {
        out.issues.push_back(ValidationIssue{
            ValidationIssue::Severity::Error,
            path + ".asset_id",
            "asset_id is required for image/video layers."
        });
        out.error_count++;
        return;
    }

    bool asset_found = false;
    for (const auto& asset : scene.assets) {
        if (asset.id == normalized.asset_reference) {
            asset_found = true;
            break;
        }
    }

    if (!asset_found && !looks_like_media_reference(std::string(normalized.asset_reference))) {
        out.issues.push_back(ValidationIssue{
            ValidationIssue::Severity::Error,
            path + ".asset_id",
            "asset id '" + std::string(normalized.asset_reference) + "' not found in scene assets."
        });
        out.error_count++;
    }
}

} // namespace tachyon::core
