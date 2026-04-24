#include "tachyon/core/spec/validation/scene_validator.h"
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>

namespace tachyon::core {

ValidationResult SceneValidator::validate(const ::tachyon::SceneSpec& scene) const {
    ValidationResult result;

    // Validate schema version first
    validate_schema_version(scene, result);

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

void SceneValidator::validate_schema_version(const ::tachyon::SceneSpec& scene, ValidationResult& out) const {
    // Check if schema version is present and valid
    if (scene.schema_version.major == 0) {
        out.issues.push_back({ValidationIssue::Severity::Warning, "scene.schema_version", 
            "Schema version is 0.0.0. This may indicate an older file format."});
        out.warning_count++;
    }
    
    // Future: Add version compatibility checks here
    // For now, we just warn about very old versions
    static const SchemaVersion min_supported{0, 9, 0};
    if (scene.schema_version < min_supported) {
        out.issues.push_back({ValidationIssue::Severity::Error, "scene.schema_version",
            "Schema version " + scene.schema_version.to_string() + " is not supported. Minimum required: " + min_supported.to_string()});
        out.error_count++;
    }
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

    // Validate duplicate IDs within composition
    validate_duplicate_ids(comp, out);
    
    // Validate camera cuts
    validate_camera_cuts(comp, out);

    for (std::size_t i = 0; i < comp.layers.size(); ++i) {
        validate_layer(comp.layers[i], comp, scene, "composition." + comp.id + ".layers[" + std::to_string(i) + "]", out);
    }
}

void SceneValidator::validate_duplicate_ids(const ::tachyon::CompositionSpec& comp, ValidationResult& out) const {
    std::unordered_set<std::string> seen_ids;
    
    // Check layer IDs
    for (std::size_t i = 0; i < comp.layers.size(); ++i) {
        const auto& layer = comp.layers[i];
        if (!layer.id.empty()) {
            if (seen_ids.count(layer.id)) {
                out.issues.push_back({ValidationIssue::Severity::Error, 
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
                out.issues.push_back({ValidationIssue::Severity::Error,
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
                out.issues.push_back({ValidationIssue::Severity::Error,
                    "composition." + comp.id + ".audio_tracks[" + std::to_string(i) + "].id",
                    "Duplicate audio track ID: " + track.id});
                out.error_count++;
            }
            seen_ids.insert(track.id);
        }
    }
}

void SceneValidator::validate_camera_cuts(const ::tachyon::CompositionSpec& comp, ValidationResult& out) const {
    // Check for overlapping camera cuts
    for (std::size_t i = 0; i < comp.camera_cuts.size(); ++i) {
        const auto& cut_i = comp.camera_cuts[i];
        
        // Validate time range
        if (cut_i.start_seconds < 0) {
            out.issues.push_back({ValidationIssue::Severity::Error,
                "composition." + comp.id + ".camera_cuts[" + std::to_string(i) + "].start_seconds",
                "Camera cut start time cannot be negative."});
            out.error_count++;
        }
        
        if (cut_i.end_seconds < cut_i.start_seconds) {
            out.issues.push_back({ValidationIssue::Severity::Error,
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
                out.issues.push_back({ValidationIssue::Severity::Error,
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
            out.issues.push_back({ValidationIssue::Severity::Error,
                "composition." + comp.id + ".camera_cuts[" + std::to_string(i) + "].camera_id",
                "Camera cut references non-existent camera: " + cut_i.camera_id});
            out.error_count++;
        }
    }
}

void SceneValidator::validate_track_bindings(const ::tachyon::LayerSpec& layer, const std::string& path, ValidationResult& out) const {
    // Validate track bindings reference valid sources
    for (std::size_t i = 0; i < layer.track_bindings.size(); ++i) {
        const auto& binding = layer.track_bindings[i];
        
        if (binding.source_id.empty()) {
            out.issues.push_back({ValidationIssue::Severity::Error,
                path + ".track_bindings[" + std::to_string(i) + "].source_id",
                "Track binding source_id cannot be empty."});
            out.error_count++;
        }
        
        if (binding.source_track_name.empty()) {
            out.issues.push_back({ValidationIssue::Severity::Error,
                path + ".track_bindings[" + std::to_string(i) + "].source_track_name",
                "Track binding source_track_name cannot be empty."});
            out.error_count++;
        }
        
        if (binding.influence < 0.0f || binding.influence > 1.0f) {
            out.issues.push_back({ValidationIssue::Severity::Warning,
                path + ".track_bindings[" + std::to_string(i) + "].influence",
                "Track binding influence should be between 0.0 and 1.0."});
            out.warning_count++;
        }
    }
}

void SceneValidator::validate_layer(const ::tachyon::LayerSpec& layer, const ::tachyon::CompositionSpec& comp, const ::tachyon::SceneSpec& scene, const std::string& path, ValidationResult& out) const {
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

    // Validate track bindings
    validate_track_bindings(layer, path, out);
    
    // Validate safe area for text layers
    validate_safe_area(layer, comp, path, out);

    // Track matte validation: if a matte layer is specified, it must exist and must not be self
    if (layer.track_matte_layer_id.has_value() && !layer.track_matte_layer_id->empty()) {
        if (*layer.track_matte_layer_id == layer.id) {
            out.issues.push_back({ValidationIssue::Severity::Error, path + ".track_matte_layer_id", "Track matte layer cannot reference itself."});
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
                out.issues.push_back({ValidationIssue::Severity::Error, path + ".track_matte_layer_id", "References non-existent layer: " + *layer.track_matte_layer_id});
                out.error_count++;
            }
        }
        // Validate that track_matte_type is not None when a matte layer is specified
        if (layer.track_matte_type == TrackMatteType::None) {
            out.issues.push_back({ValidationIssue::Severity::Warning, path + ".track_matte_type", "track_matte_layer_id is set but track_matte_type is None."});
        }
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

void SceneValidator::validate_safe_area(const ::tachyon::LayerSpec& layer, const ::tachyon::CompositionSpec& comp, const std::string& path, ValidationResult& out) const {
    // Safe area YouTube = 10% margine su tutti i lati
    // Se layer ha testo E posizione fuori dall'area sicura → Warning
    
    // Applica solo a layer di tipo text
    if (layer.type != "text") {
        return;
    }
    
    const float safe_x = comp.width * 0.10f;
    const float safe_y = comp.height * 0.10f;
    const float safe_width = comp.width - 2.0f * safe_x;
    const float safe_height = comp.height - 2.0f * safe_y;
    
    // Calcola bounding box del layer
    const float layer_left = layer.transform.position.x;
    const float layer_top = layer.transform.position.y;
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
        out.issues.push_back({ValidationIssue::Severity::Warning, 
            path + ".transform.position",
            "Text layer may be outside YouTube safe area (10% margin). " + violation_details});
        out.warning_count++;
    }
}

// void SceneValidator::validate_property removed: PropertySpec type no longer exists
    // Base property validation hook. The concrete property schema is still
    // normalized elsewhere, so this stays intentionally shallow for now.

void SceneValidator::check_cycles(const ::tachyon::SceneSpec& scene, ValidationResult& out) const {
    // Check matte dependency cycles within each composition
    for (const auto& comp : scene.compositions) {
        // Build matte adjacency graph: source -> [targets]
        std::unordered_map<std::string, std::vector<std::string>> matte_adj;
        std::unordered_map<std::string, int> matte_in_degree;
        std::unordered_set<std::string> layer_ids;
        for (const auto& l : comp.layers) {
            if (!l.id.empty()) layer_ids.insert(l.id);
        }

        for (const auto& layer : comp.layers) {
            if (layer.track_matte_layer_id.has_value() && !layer.track_matte_layer_id->empty()) {
                const std::string& src = *layer.track_matte_layer_id;
                const std::string& tgt = layer.id;
                if (layer_ids.count(src) && layer_ids.count(tgt)) {
                    matte_adj[src].push_back(tgt);
                    matte_in_degree[tgt]++;
                    if (matte_in_degree.find(src) == matte_in_degree.end()) {
                        matte_in_degree[src] = 0;
                    }
                }
            }
        }

        // Kahn's algorithm for cycle detection
        std::queue<std::string> q;
        for (const auto& [id, deg] : matte_in_degree) {
            if (deg == 0) q.push(id);
        }
        std::size_t processed = 0;
        while (!q.empty()) {
            std::string cur = q.front(); q.pop();
            processed++;
            auto it = matte_adj.find(cur);
            if (it != matte_adj.end()) {
                for (const auto& neighbor : it->second) {
                    if (--matte_in_degree[neighbor] == 0) {
                        q.push(neighbor);
                    }
                }
            }
        }
        if (processed != matte_in_degree.size()) {
            out.issues.push_back({ValidationIssue::Severity::Error, "composition." + comp.id + ".matte", "Cycle detected in track matte dependencies."});
            out.error_count++;
        }
    }

    // Check precomp reference cycles across compositions
    std::unordered_map<std::string, std::vector<std::string>> precomp_adj;
    std::unordered_map<std::string, int> precomp_in_degree;
    std::unordered_set<std::string> comp_ids;
    for (const auto& c : scene.compositions) {
        if (!c.id.empty()) comp_ids.insert(c.id);
    }

    for (const auto& comp : scene.compositions) {
        for (const auto& layer : comp.layers) {
            if (layer.type == "precomp" && layer.precomp_id.has_value() && !layer.precomp_id->empty()) {
                const std::string& src = comp.id;
                const std::string& tgt = *layer.precomp_id;
                if (comp_ids.count(src) && comp_ids.count(tgt)) {
                    precomp_adj[src].push_back(tgt);
                    precomp_in_degree[tgt]++;
                    if (precomp_in_degree.find(src) == precomp_in_degree.end()) {
                        precomp_in_degree[src] = 0;
                    }
                }
            }
        }
    }

    std::queue<std::string> q2;
    for (const auto& [id, deg] : precomp_in_degree) {
        if (deg == 0) q2.push(id);
    }
    std::size_t processed2 = 0;
    while (!q2.empty()) {
        std::string cur = q2.front(); q2.pop();
        processed2++;
        auto it = precomp_adj.find(cur);
        if (it != precomp_adj.end()) {
            for (const auto& neighbor : it->second) {
                if (--precomp_in_degree[neighbor] == 0) {
                    q2.push(neighbor);
                }
            }
        }
    }
    if (processed2 != precomp_in_degree.size()) {
        out.issues.push_back({ValidationIssue::Severity::Error, "scene.precomp", "Cycle detected in precomp references."});
        out.error_count++;
    }
}

} // namespace tachyon::core
