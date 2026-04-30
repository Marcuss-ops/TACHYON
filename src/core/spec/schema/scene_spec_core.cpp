#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <fstream>
#include <set>
#include <sstream>
#include <algorithm>

namespace tachyon {

std::string make_path(const std::string& parent, const std::string& child) {
    if (parent.empty()) return child;
    if (child.empty()) return parent;
    return parent + "." + child;
}

bool is_asset_alpha_mode_valid(const std::string& mode) {
    return mode == "premultiplied" || mode == "straight" || mode == "opaque";
}

bool is_layer_blend_mode_valid(const std::string& mode) {
    return mode == "normal" || mode == "additive" || mode == "multiply" || mode == "screen" || mode == "overlay" || mode == "soft_light" || mode == "softLight";
}

bool looks_like_media_path(const std::string& value) {
    if (value.empty()) {
        return false;
    }

    const std::filesystem::path path(value);
    if (path.has_extension()) {
        return true;
    }

    return value.find('/') != std::string::npos || value.find('\\') != std::string::npos;
}

ValidationResult validate_scene_spec(const SceneSpec& scene) {
    ValidationResult result;
    if (scene.project.id.empty()) result.diagnostics.add_error("scene.project.id_missing", "project.id is required", "project.id");
    if (scene.compositions.empty()) result.diagnostics.add_error("scene.compositions.missing", "at least one composition is required", "compositions");

    std::set<std::string> asset_ids;
    for (std::size_t i = 0; i < scene.assets.size(); ++i) {
        const auto& asset = scene.assets[i];
        const std::string apath = "assets[" + std::to_string(i) + "]";
        if (asset.id.empty()) {
            result.diagnostics.add_error("scene.asset.id_missing", "id required", apath + ".id");
        } else if (!asset_ids.insert(asset.id).second) {
            result.diagnostics.add_error("scene.asset.id_duplicate", "asset id must be unique", apath + ".id");
        }
    }

    std::set<std::string> composition_ids;
    for (const auto& comp : scene.compositions) {
        composition_ids.insert(comp.id);
    }

    for (std::size_t i = 0; i < scene.compositions.size(); ++i) {
        const auto& comp = scene.compositions[i];
        std::string path = "compositions[" + std::to_string(i) + "]";
        if (comp.id.empty()) result.diagnostics.add_error("scene.composition.id_missing", "id required", path + ".id");
        if (comp.width <= 0 || comp.height <= 0) result.diagnostics.add_error("scene.composition.size_invalid", "size must be positive", path + ".width");

        std::set<std::string> layer_ids;
        for (std::size_t j = 0; j < comp.layers.size(); ++j) {
            const auto& layer = comp.layers[j];
            const std::string lpath = path + ".layers[" + std::to_string(j) + "]";
            if (layer.id.empty()) {
                continue;
            }
            if (!layer_ids.insert(layer.id).second) {
                result.diagnostics.add_error("scene.layer.id_duplicate", "layer id must be unique within a composition", lpath + ".id");
            }
        }

        for (std::size_t j = 0; j < comp.layers.size(); ++j) {
            const auto& layer = comp.layers[j];
            std::string lpath = path + ".layers[" + std::to_string(j) + "]";
            
            if (layer.id.empty()) result.diagnostics.add_error("scene.layer.id_missing", "id required", lpath + ".id");
            if (!is_layer_blend_mode_valid(layer.blend_mode)) result.diagnostics.add_error("scene.layer.blend_mode_invalid", "blend_mode invalid", lpath + ".blend_mode");

            // Parenting Validation with Cycle Detection
            if (layer.parent.has_value() && !layer.parent->empty()) {
                if (*layer.parent == layer.id) {
                    result.diagnostics.add_error("scene.layer.parent_cycle.direct", "layer.parent cannot reference itself", lpath + ".parent");
                } else if (!layer_ids.count(*layer.parent)) {
                    result.diagnostics.add_error("scene.layer.parent_invalid", "layer.parent must reference an existing layer id", lpath + ".parent");
                } else {
                    // Industrial cycle detection
                    std::string current = *layer.parent;
                    std::set<std::string> visited = { layer.id };
                    bool cycle = false;
                    while (true) {
                        if (visited.count(current)) {
                            cycle = true;
                            break;
                        }
                        visited.insert(current);
                        auto pit = std::find_if(comp.layers.begin(), comp.layers.end(), [&](const auto& l) { return l.id == current; });
                        if (pit != comp.layers.end() && pit->parent.has_value() && !pit->parent->empty()) {
                            current = *pit->parent;
                        } else {
                            break;
                        }
                    }
                    if (cycle) {
                        result.diagnostics.add_error("scene.layer.parent_cycle.indirect", "indirect cycle detected in parenting", lpath + ".parent");
                    }
                }
            }

            if (layer.track_matte_layer_id.has_value() && !layer.track_matte_layer_id->empty() && !layer_ids.count(*layer.track_matte_layer_id)) {
                result.diagnostics.add_error("scene.layer.track_matte_layer_id_invalid", "track_matte_layer_id invalid", lpath + ".track_matte_layer_id");
            }

            if (layer.precomp_id.has_value() && !layer.precomp_id->empty() && !composition_ids.count(*layer.precomp_id)) {
                result.diagnostics.add_error("scene.layer.precomp_id_invalid", "precomp_id invalid", lpath + ".precomp_id");
            }

            if (layer.type == "image" || layer.type == "video") {
                bool asset_found = false;
                for (const auto& asset : scene.assets) {
                    if (asset.id == layer.id || asset.id == layer.name) {
                        asset_found = true;
                        break;
                    }
                }
                if (!asset_found && !looks_like_media_path(layer.name)) {
                    result.diagnostics.add_error("scene.layer.asset_reference_invalid", "asset not found", lpath + ".asset_id");
                }
            }
        }
    }

    // Warning if font manifest missing but text layers present
    if (!scene.font_manifest.has_value()) {
        bool has_text_layer = false;
        for (const auto& comp : scene.compositions) {
            for (const auto& layer : comp.layers) {
                if (layer.type == "text") {
                    has_text_layer = true;
                    break;
                }
            }
            if (has_text_layer) break;
        }
        if (has_text_layer) {
            result.diagnostics.add_warning(
                "scene.font_manifest.missing",
                "font_manifest not provided, but text layers are present; fonts may not load correctly"
            );
        }
    }

    return result;
}

} // namespace tachyon
