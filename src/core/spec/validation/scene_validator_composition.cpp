#include "tachyon/core/spec/validation/scene_validator.h"
#include <unordered_set>

namespace tachyon::core {

void SceneValidator::validate_composition(const NormalizedCompositionView& comp, const ::tachyon::SceneSpec& scene, ValidationResult& out) const {
    const auto* source = comp.source;
    if (source == nullptr) {
        out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Fatal, "composition", "Normalized composition view is missing source data."});
        out.fatal_count++;
        return;
    }

    if (source->id.empty()) {
        out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, "composition", "Composition ID cannot be empty."});
        out.error_count++;
    }
    
    if (source->width <= 0 || source->height <= 0) {
        out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, "composition." + source->id + ".dimensions", "Invalid dimensions."});
        out.error_count++;
    }

    if (source->fps <= 0.0f) {
        out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, "composition." + source->id + ".fps", "FPS must be greater than 0."});
        out.error_count++;
    }

    if (source->duration <= 0.0f) {
        out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, "composition." + source->id + ".duration", "Duration must be greater than 0."});
        out.error_count++;
    }

    validate_duplicate_ids(*source, out);

    for (std::size_t i = 0; i < comp.layers.size(); ++i) {
        validate_layer(comp.layers[i], *source, scene, "composition." + source->id + ".layers[" + std::to_string(i) + "]", out);
    }
}

void SceneValidator::validate_composition(const ::tachyon::CompositionSpec& comp, const ::tachyon::SceneSpec& scene, ValidationResult& out) const {
    validate_composition(normalize_composition_view(comp), scene, out);
}

void SceneValidator::validate_duplicate_ids(const ::tachyon::CompositionSpec& comp, ValidationResult& out) const {
    std::unordered_set<std::string> seen_ids;
    
    // Check layer IDs
    for (std::size_t i = 0; i < comp.layers.size(); ++i) {
        const auto& layer = comp.layers[i];
        if (!layer.id.empty()) {
            if (seen_ids.count(layer.id)) {
                out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, 
                    "composition." + comp.id + ".layers[" + std::to_string(i) + "].id",
                    "Duplicate layer ID: " + layer.id});
                out.error_count++;
            }
            seen_ids.insert(layer.id);
        }
    }
    
    // Check camera IDs
    for (std::size_t i = 0; i < comp.cameras_2d.size(); ++i) {
        const auto& cam = comp.cameras_2d[i];
        if (!cam.id.empty()) {
            if (seen_ids.count(cam.id)) {
                out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error,
                    "composition." + comp.id + ".cameras_2d[" + std::to_string(i) + "].id",
                    "Duplicate camera ID: " + cam.id});
                out.error_count++;
            }
            seen_ids.insert(cam.id);
        }
    }
    
    // Check audio track IDs
    for (std::size_t i = 0; i < comp.audio_tracks.size(); ++i) {
        const auto& track = comp.audio_tracks[i];
        if (!track.id.empty()) {
            if (seen_ids.count(track.id)) {
                out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error,
                    "composition." + comp.id + ".audio_tracks[" + std::to_string(i) + "].id",
                    "Duplicate audio track ID: " + track.id});
                out.error_count++;
            }
            seen_ids.insert(track.id);
        }
    }

    // Validate audio file references
    validate_audio_files(comp, "composition." + comp.id, out);
}


} // namespace tachyon::core
