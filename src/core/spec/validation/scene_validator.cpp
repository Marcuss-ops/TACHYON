#include "tachyon/core/spec/validation/scene_validator.h"
#include <unordered_map>
#include <unordered_set>
#include <queue>

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
