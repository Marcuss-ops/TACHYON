#include "tachyon/core/spec/validation/scene_validator.h"

namespace tachyon::core {

ValidationResult SceneValidator::validate(const ::tachyon::SceneSpec& scene) const {
    ValidationResult result;

    if (scene.compositions.empty()) {
        result.issues.push_back({ValidationIssue::Severity::Error, "scene", "Scene has no compositions."});
        result.error_count++;
    }

    for (std::size_t i = 0; i < scene.compositions.size(); ++i) {
        validate_composition(scene.compositions[i], scene, result);
    }

    check_cycles(scene, result);

    return result;
}

void SceneValidator::validate_composition(const ::tachyon::CompositionSpec& comp, const ::tachyon::SceneSpec& scene, ValidationResult& out) const {
    if (comp.id.empty()) {
        out.issues.push_back({ValidationIssue::Severity::Error, "composition", "Composition ID cannot be empty."});
        out.error_count++;
    }
    
    if (comp.width <= 0 || comp.height <= 0) {
        out.issues.push_back({ValidationIssue::Severity::Error, "composition." + comp.id + ".dimensions", "Invalid dimensions."});
        out.error_count++;
    }

    for (std::size_t i = 0; i < comp.layers.size(); ++i) {
        validate_layer(comp.layers[i], comp, scene, "composition." + comp.id + ".layers[" + std::to_string(i) + "]", out);
    }
}

void SceneValidator::validate_layer(const ::tachyon::LayerSpec& layer, const ::tachyon::CompositionSpec&, const ::tachyon::SceneSpec& scene, const std::string& path, ValidationResult& out) const {
    if (layer.id.empty()) {
        out.issues.push_back({ValidationIssue::Severity::Error, path + ".id", "Layer ID cannot be empty."});
        out.error_count++;
    }

    if (layer.opacity < 0.0 || layer.opacity > 1.0) {
        out.issues.push_back({ValidationIssue::Severity::Error, path + ".opacity", "Layer opacity must be between 0 and 1."});
        out.error_count++;
    }

    if (layer.width < 0 || layer.height < 0) {
        out.issues.push_back({ValidationIssue::Severity::Error, path + ".dimensions", "Layer dimensions cannot be negative."});
        out.error_count++;
    }

    if (layer.type == "precomp") {
        if (!layer.precomp_id.has_value() || layer.precomp_id->empty()) {
            out.issues.push_back({ValidationIssue::Severity::Error, path + ".precomp_id", "Precomp layer requires a composition reference."});
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
                out.issues.push_back({ValidationIssue::Severity::Error, path + ".precomp_id", "References non-existent composition: " + *layer.precomp_id});
                out.error_count++;
            }
        }
    }
}

void SceneValidator::validate_property(const ::tachyon::PropertySpec&, const std::string&, ValidationResult&) const {
    // Base property validation hook. The concrete property schema is still
    // normalized elsewhere, so this stays intentionally shallow for now.
}

void SceneValidator::check_cycles(const ::tachyon::SceneSpec&, ValidationResult&) const {
    // DAG cycle detection will be added once precomp references become part of
    // the runtime dependency graph. For now, precomp existence is validated
    // during layer validation.
}

} // namespace tachyon::core
