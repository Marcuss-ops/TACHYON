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
    validate_camera_cuts(*source, out);

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

void SceneValidator::validate_camera_cuts(const ::tachyon::CompositionSpec& comp, ValidationResult& out) const {
    // Check for overlapping camera cuts
    for (std::size_t i = 0; i < comp.camera_cuts.size(); ++i) {
        const auto& cut_i = comp.camera_cuts[i];
        
        // Validate time range
        if (cut_i.start_seconds < 0) {
            out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error,
                "composition." + comp.id + ".camera_cuts[" + std::to_string(i) + "].start_seconds",
                "Camera cut start time cannot be negative."});
            out.error_count++;
        }
        
        if (cut_i.end_seconds < cut_i.start_seconds) {
            out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error,
                "composition." + comp.id + ".camera_cuts[" + std::to_string(i) + "]",
                "Camera cut end time must be >= start time."});
            out.error_count++;
        }
        
        // Check for overlaps with other cuts
        for (std::size_t j = i + 1; j < comp.camera_cuts.size(); ++j) {
            const auto& cut_j = comp.camera_cuts[j];
            
            // Two cuts overlap if one starts before the other ends
            bool overlaps = (cut_i.start_seconds < cut_j.end_seconds) && (cut_j.start_seconds < cut_i.end_seconds);
            
            if (overlaps) {
                out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error,
                    "composition." + comp.id + ".camera_cuts",
                    "Overlapping camera cuts at indices " + std::to_string(i) + " and " + std::to_string(j)});
                out.error_count++;
            }
        }
        
        // Validate camera reference exists
        bool camera_found = false;
        for (const auto& cam : comp.cameras_2d) {
            if (cam.id == cut_i.camera_id) {
                camera_found = true;
                break;
            }
        }
        if (!camera_found && !cut_i.camera_id.empty()) {
            out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error,
                "composition." + comp.id + ".camera_cuts[" + std::to_string(i) + "].camera_id",
                "Camera cut references non-existent camera: " + cut_i.camera_id});
            out.error_count++;
        }
    }
}

} // namespace tachyon::core
