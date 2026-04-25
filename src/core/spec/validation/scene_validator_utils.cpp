#include "tachyon/core/spec/validation/scene_validator.h"
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <fstream>

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
            out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, "composition." + comp.id + ".matte", "Cycle detected in track matte dependencies."});
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
        out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error, "scene.precomp", "Cycle detected in precomp references."});
        out.error_count++;
    }
}

void SceneValidator::validate_audio_files(const ::tachyon::CompositionSpec& comp, const std::string& path, ValidationResult& out) const {
    for (std::size_t i = 0; i < comp.audio_tracks.size(); ++i) {
        const auto& track = comp.audio_tracks[i];
        if (!track.source_path.empty()) {
            std::ifstream file(track.source_path);
            if (!file.good()) {
                out.issues.push_back(ValidationIssue{ValidationIssue::Severity::Error,
                    path + ".audio_tracks[" + std::to_string(i) + "].source_path",
                    "Audio file not found: " + track.source_path});
                out.error_count++;
            }
        }
    }
}

std::size_t SceneValidator::estimate_memory(const ::tachyon::SceneSpec& scene) const {
    std::size_t total = 0;
    
    for (const auto& comp : scene.compositions) {
        // Estimate framebuffer memory: width * height * 4 bytes/pixel * some overhead
        std::size_t framebuffer_bytes = static_cast<std::size_t>(comp.width) * 
                                       static_cast<std::size_t>(comp.height) * 4ULL * 2ULL; // double-buffered
        total += framebuffer_bytes;
        
        // Estimate layer memory
        total += comp.layers.size() * 1024; // ~1KB per layer overhead
        
        // Estimate audio memory (assuming 2 channels, 48kHz, 4 bytes/sample)
        for (const auto& track : comp.audio_tracks) {
            // Calculate duration from trim points
            double duration_seconds = 0.0;
            if (track.out_point_seconds > track.in_point_seconds) {
                duration_seconds = track.out_point_seconds - track.in_point_seconds;
            } else if (track.out_point_seconds == -1.0) {
                // Full duration unknown, skip memory estimation for this track
                continue;
            }
            std::size_t duration_samples = static_cast<std::size_t>(duration_seconds * 48000.0f);
            total += duration_samples * 2 * 4; // stereo, 32-bit float
        }
    }
    
    return total;
}

} // namespace tachyon::core
