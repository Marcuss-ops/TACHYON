#include "tachyon/renderer2d/evaluated_composition/renderer2d_matte_resolver.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <queue>

namespace tachyon::renderer2d {

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

bool Renderer2DMatteResolver::validate(
    const std::vector<scene::EvaluatedLayerState>& layers,
    const std::vector<MatteDependency>& dependencies,
    std::string& out_error) const {
    
    // Build ID -> index map
    std::unordered_map<std::string, std::size_t> id_to_index;
    for (std::size_t i = 0; i < layers.size(); ++i) {
        if (!layers[i].id.empty()) {
            id_to_index[layers[i].id] = i;
        }
    }
    
    // Check all IDs exist
    for (const auto& dep : dependencies) {
        if (id_to_index.find(dep.source_layer_id) == id_to_index.end()) {
            out_error = "Matte source layer not found: " + dep.source_layer_id;
            return false;
        }
        if (id_to_index.find(dep.target_layer_id) == id_to_index.end()) {
            out_error = "Matte target layer not found: " + dep.target_layer_id;
            return false;
        }
    }
    
    // Check self-matting
    for (const auto& dep : dependencies) {
        if (dep.source_layer_id == dep.target_layer_id) {
            out_error = "Self-matting not allowed: " + dep.source_layer_id;
            return false;
        }
    }
    
    // Check cycles using Kahn's algorithm
    std::unordered_map<std::string, std::vector<std::string>> adjacency;
    std::unordered_map<std::string, int> in_degree;
    
    for (const auto& dep : dependencies) {
        adjacency[dep.source_layer_id].push_back(dep.target_layer_id);
        in_degree[dep.target_layer_id]++;
        if (in_degree.find(dep.source_layer_id) == in_degree.end()) {
            in_degree[dep.source_layer_id] = 0;
        }
    }
    
    std::queue<std::string> queue;
    for (const auto& [id, degree] : in_degree) {
        if (degree == 0) queue.push(id);
    }
    
    std::size_t processed = 0;
    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop();
        processed++;
        
        auto it = adjacency.find(current);
        if (it != adjacency.end()) {
            for (const auto& neighbor : it->second) {
                in_degree[neighbor]--;
                if (in_degree[neighbor] == 0) {
                    queue.push(neighbor);
                }
            }
        }
    }
    
    if (processed != in_degree.size()) {
        out_error = "Cycle detected in matte dependencies";
        return false;
    }
    
    return true;
}

// ---------------------------------------------------------------------------
// Resolution
// ---------------------------------------------------------------------------

static float rgb_to_luma(float r, float g, float b) {
    // Rec. 709 luma coefficients in linear space
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

void Renderer2DMatteResolver::resolve_alpha_matte(
    const std::vector<float>& source_a,
    std::vector<float>& out_matte,
    bool invert) const {
    
    const std::size_t pixel_count = source_a.size();
    out_matte.resize(pixel_count);
    
    if (invert) {
        for (std::size_t i = 0; i < pixel_count; ++i) {
            out_matte[i] = 1.0f - source_a[i];
        }
    } else {
        for (std::size_t i = 0; i < pixel_count; ++i) {
            out_matte[i] = source_a[i];
        }
    }
}

void Renderer2DMatteResolver::resolve_luma_matte(
    const std::vector<float>& source_r,
    const std::vector<float>& source_g,
    const std::vector<float>& source_b,
    std::vector<float>& out_matte,
    bool invert) const {
    
    const std::size_t pixel_count = source_r.size();
    out_matte.resize(pixel_count);
    
    if (invert) {
        for (std::size_t i = 0; i < pixel_count; ++i) {
            float luma = rgb_to_luma(source_r[i], source_g[i], source_b[i]);
            out_matte[i] = 1.0f - luma;
        }
    } else {
        for (std::size_t i = 0; i < pixel_count; ++i) {
            float luma = rgb_to_luma(source_r[i], source_g[i], source_b[i]);
            out_matte[i] = luma;
        }
    }
}

void Renderer2DMatteResolver::resolve(
    const std::vector<scene::EvaluatedLayerState>& layers,
    const std::vector<MatteDependency>& dependencies,
    const std::unordered_map<std::string, std::shared_ptr<SurfaceRGBA>>& rendered_surfaces,
    std::vector<std::vector<float>>& out_matte_buffers,
    std::int64_t width,
    std::int64_t height) const {

    const std::size_t layer_count = layers.size();
    const std::size_t pixel_count = static_cast<std::size_t>(width * height);

    // Initialize all matte buffers to 1.0f (fully opaque)
    out_matte_buffers.resize(layer_count);
    for (std::size_t i = 0; i < layer_count; ++i) {
        out_matte_buffers[i].resize(pixel_count, 1.0f);
    }

    if (dependencies.empty()) return;

    // Build ID -> index map
    std::unordered_map<std::string, std::size_t> id_to_index;
    for (std::size_t i = 0; i < layer_count; ++i) {
        if (!layers[i].id.empty()) {
            id_to_index[layers[i].id] = i;
        }
    }

    // Build dependency graph for topological order
    std::unordered_map<std::string, std::vector<std::string>> adjacency;
    std::unordered_map<std::string, int> in_degree;
    std::unordered_map<std::string, MatteDependency> dep_map;

    for (const auto& dep : dependencies) {
        // Skip self-matte (should have been caught in validation, but be safe)
        if (dep.source_layer_id == dep.target_layer_id) continue;
        
        // Skip if source or target not found
        if (id_to_index.find(dep.source_layer_id) == id_to_index.end()) continue;
        if (id_to_index.find(dep.target_layer_id) == id_to_index.end()) continue;
        
        adjacency[dep.source_layer_id].push_back(dep.target_layer_id);
        in_degree[dep.target_layer_id]++;
        if (in_degree.find(dep.source_layer_id) == in_degree.end()) {
            in_degree[dep.source_layer_id] = 0;
        }
        dep_map[dep.target_layer_id] = dep;
    }

    // Topological sort (Kahn's algorithm)
    std::queue<std::string> queue;
    for (const auto& [id, degree] : in_degree) {
        if (degree == 0) queue.push(id);
    }

    std::vector<std::string> topo_order;
    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop();
        topo_order.push_back(current);

        auto it = adjacency.find(current);
        if (it != adjacency.end()) {
            for (const auto& neighbor : it->second) {
                in_degree[neighbor]--;
                if (in_degree[neighbor] == 0) {
                    queue.push(neighbor);
                }
            }
        }
    }

    // Resolve matte sources first, then targets
    // For each layer in topo order, if it's a target, resolve its matte
    for (const auto& layer_id : topo_order) {
        auto dep_it = dep_map.find(layer_id);
        if (dep_it == dep_map.end()) continue; // Not a target

        const auto& dep = dep_it->second;
        auto src_it = id_to_index.find(dep.source_layer_id);
        auto tgt_it = id_to_index.find(dep.target_layer_id);

        if (src_it == id_to_index.end() || tgt_it == id_to_index.end()) continue;

        std::size_t src_idx = src_it->second;
        std::size_t tgt_idx = tgt_it->second;

        // Get the actual rendered surface from the source layer
        auto surface_it = rendered_surfaces.find(dep.source_layer_id);
        
        // Fallback: if no surface available, use layer opacity
        if (surface_it == rendered_surfaces.end() || !surface_it->second) {
            // Create matte from layer opacity
            const float opacity = std::clamp(static_cast<float>(layers[src_idx].opacity), 0.0f, 1.0f);
            std::vector<float> source_a(pixel_count, opacity);
            switch (dep.mode) {
                case MatteMode::Alpha:
                case MatteMode::AlphaInverted:
                    resolve_alpha_matte(source_a, out_matte_buffers[tgt_idx], dep.mode == MatteMode::AlphaInverted);
                    break;
                case MatteMode::Luma:
                case MatteMode::LumaInverted: {
                    // For luma fallback, treat opacity as luma value
                    std::vector<float> source_r(pixel_count, opacity);
                    std::vector<float> source_g(pixel_count, opacity);
                    std::vector<float> source_b(pixel_count, opacity);
                    resolve_luma_matte(source_r, source_g, source_b, out_matte_buffers[tgt_idx], dep.mode == MatteMode::LumaInverted);
                    break;
                }
                default:
                    break;
            }
            continue;
        }

        const auto& surface = *(surface_it->second);
        const std::int64_t surf_w = surface.width();
        const std::int64_t surf_h = surface.height();

        // Check if surface dimensions match expected dimensions
        if (surf_w <= 0 || surf_h <= 0) {
            // Invalid surface, use default matte (fully opaque)
            out_matte_buffers[tgt_idx].assign(pixel_count, 1.0f);
            continue;
        }

        switch (dep.mode) {
            case MatteMode::Alpha: {
                std::vector<float> source_a(pixel_count);
                for (std::size_t i = 0; i < pixel_count; ++i) {
                    std::int64_t x = static_cast<std::int64_t>(i) % width;
                    std::int64_t y = static_cast<std::int64_t>(i) / width;
                    // Sample from surface with bounds checking and proper coordinate mapping
                    float u = (width > 0) ? static_cast<float>(x) / static_cast<float>(width) : 0.5f;
                    float v = (height > 0) ? static_cast<float>(y) / static_cast<float>(height) : 0.5f;
                    std::int64_t src_x = static_cast<std::int64_t>(u * static_cast<float>(surf_w));
                    std::int64_t src_y = static_cast<std::int64_t>(v * static_cast<float>(surf_h));
                    src_x = std::clamp(src_x, static_cast<std::int64_t>(0), surf_w - 1);
                    src_y = std::clamp(src_y, static_cast<std::int64_t>(0), surf_h - 1);
                    source_a[i] = surface.get_pixel(static_cast<std::uint32_t>(src_x), static_cast<std::uint32_t>(src_y)).a;
                }
                resolve_alpha_matte(source_a, out_matte_buffers[tgt_idx], false);
                break;
            }
            case MatteMode::AlphaInverted: {
                std::vector<float> source_a(pixel_count);
                for (std::size_t i = 0; i < pixel_count; ++i) {
                    std::int64_t x = static_cast<std::int64_t>(i) % width;
                    std::int64_t y = static_cast<std::int64_t>(i) / width;
                    float u = (width > 0) ? static_cast<float>(x) / static_cast<float>(width) : 0.5f;
                    float v = (height > 0) ? static_cast<float>(y) / static_cast<float>(height) : 0.5f;
                    std::int64_t src_x = static_cast<std::int64_t>(u * static_cast<float>(surf_w));
                    std::int64_t src_y = static_cast<std::int64_t>(v * static_cast<float>(surf_h));
                    src_x = std::clamp(src_x, static_cast<std::int64_t>(0), surf_w - 1);
                    src_y = std::clamp(src_y, static_cast<std::int64_t>(0), surf_h - 1);
                    source_a[i] = surface.get_pixel(static_cast<std::uint32_t>(src_x), static_cast<std::uint32_t>(src_y)).a;
                }
                resolve_alpha_matte(source_a, out_matte_buffers[tgt_idx], true);
                break;
            }
            case MatteMode::Luma: {
                std::vector<float> source_r(pixel_count);
                std::vector<float> source_g(pixel_count);
                std::vector<float> source_b(pixel_count);
                for (std::size_t i = 0; i < pixel_count; ++i) {
                    std::int64_t x = static_cast<std::int64_t>(i) % width;
                    std::int64_t y = static_cast<std::int64_t>(i) / width;
                    float u = (width > 0) ? static_cast<float>(x) / static_cast<float>(width) : 0.5f;
                    float v = (height > 0) ? static_cast<float>(y) / static_cast<float>(height) : 0.5f;
                    std::int64_t src_x = static_cast<std::int64_t>(u * static_cast<float>(surf_w));
                    std::int64_t src_y = static_cast<std::int64_t>(v * static_cast<float>(surf_h));
                    src_x = std::clamp(src_x, static_cast<std::int64_t>(0), surf_w - 1);
                    src_y = std::clamp(src_y, static_cast<std::int64_t>(0), surf_h - 1);
                    const auto px = surface.get_pixel(static_cast<std::uint32_t>(src_x), static_cast<std::uint32_t>(src_y));
                    source_r[i] = px.r;
                    source_g[i] = px.g;
                    source_b[i] = px.b;
                }
                resolve_luma_matte(source_r, source_g, source_b, out_matte_buffers[tgt_idx], false);
                break;
            }
            case MatteMode::LumaInverted: {
                std::vector<float> source_r(pixel_count);
                std::vector<float> source_g(pixel_count);
                std::vector<float> source_b(pixel_count);
                for (std::size_t i = 0; i < pixel_count; ++i) {
                    std::int64_t x = static_cast<std::int64_t>(i) % width;
                    std::int64_t y = static_cast<std::int64_t>(i) / width;
                    float u = (width > 0) ? static_cast<float>(x) / static_cast<float>(width) : 0.5f;
                    float v = (height > 0) ? static_cast<float>(y) / static_cast<float>(height) : 0.5f;
                    std::int64_t src_x = static_cast<std::int64_t>(u * static_cast<float>(surf_w));
                    std::int64_t src_y = static_cast<std::int64_t>(v * static_cast<float>(surf_h));
                    src_x = std::clamp(src_x, static_cast<std::int64_t>(0), surf_w - 1);
                    src_y = std::clamp(src_y, static_cast<std::int64_t>(0), surf_h - 1);
                    const auto px = surface.get_pixel(static_cast<std::uint32_t>(src_x), static_cast<std::uint32_t>(src_y));
                    source_r[i] = px.r;
                    source_g[i] = px.g;
                    source_b[i] = px.b;
                }
                resolve_luma_matte(source_r, source_g, source_b, out_matte_buffers[tgt_idx], true);
                break;
            }
            case MatteMode::None:
            default:
                out_matte_buffers[tgt_idx].assign(pixel_count, 1.0f);
                break;
        }
    }
}

} // namespace tachyon::renderer2d
