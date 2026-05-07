#include "tachyon/renderer3d/utilities/debug_tools.h"
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace tachyon::renderer3d {

RendererDebug::RendererDebug() {
    config_.show_bounding_boxes = false;
    config_.show_wireframe = false;
    config_.show_normals = false;
    config_.show_mesh_axes = false;
    config_.show_light_positions = false;
    config_.show_camera_info = false;
    selected_layer_ = "";
    display_mode_ = "full";
}

void RendererDebug::set_config(const VisualizerConfig& config) {
    config_ = config;
}

RendererDebug::DebugOverlay RendererDebug::generate_overlay(const scene::EvaluatedCompositionState& state, const render::RenderIntent* intent) {
    DebugOverlay overlay;

    if (config_.show_bounding_boxes) {
        for (const auto& layer : state.layers) {
            if (!layer.is_3d) continue;

            DebugOverlay::BoundingBoxDebug bb;
            bb.layer_id = layer.id;
            bb.box = SceneCulling::BoundingBox::from_center_extents(layer.world_position3, layer.scale_3d);
            bb.color[0] = config_.bounding_box_color[0];
            bb.color[1] = config_.bounding_box_color[1];
            bb.color[2] = config_.bounding_box_color[2];
            bb.is_selected = (layer.id == selected_layer_);

            if (bb.is_selected) {
                bb.color[0] = config_.selected_color[0];
                bb.color[1] = config_.selected_color[1];
                bb.color[2] = config_.selected_color[2];
            }

            overlay.bounding_boxes.push_back(bb);
        }
    }

    if (config_.show_normals) {
        for (const auto& layer : state.layers) {
            if (!layer.is_3d) continue;

            const media::MeshAsset* mesh = nullptr;
            if (intent) {
                auto it = intent->layer_resources.find(layer.id);
                if (it != intent->layer_resources.end()) {
                    mesh = it->second.mesh_asset.get();
                }
            }

            if (!mesh) continue;

            for (const auto& sub : mesh->sub_meshes) {
                if (sub.vertices.empty()) continue;

                DebugOverlay::NormalDebug nd;
                nd.layer_id = layer.id;
                nd.position = sub.vertices[0].position;
                nd.normal = sub.vertices[0].normal;
                nd.length = 1.0f;

                overlay.normals.push_back(nd);
            }
        }
    }

    if (config_.show_light_positions) {
        for (const auto& light : state.lights) {
            DebugOverlay::TextLabel label;
            label.text = light.type + " (" + std::to_string(light.intensity) + ")";
            label.position = light.position;
            label.color[0] = 1.0f;
            label.color[1] = 1.0f;
            label.color[2] = 0.0f;
            label.size = 10.0f;

            overlay.labels.push_back(label);
        }
    }

    if (config_.show_camera_info && state.camera.available) {
        DebugOverlay::TextLabel label;
        label.text = "Camera: " + state.camera.layer_id;
        label.position = state.camera.position;
        label.color[0] = 0.0f;
        label.color[1] = 1.0f;
        label.color[2] = 1.0f;
        label.size = 12.0f;

        overlay.labels.push_back(label);
    }

    return overlay;
}

void RendererDebug::set_selected_layer(const std::string& layer_id) {
    selected_layer_ = layer_id;
}

void RendererDebug::set_display_mode(const std::string& mode) {
    display_mode_ = mode;
}

void RendererDebug::ProfilerData::begin_section(const std::string& name) {
    auto it = section_indices.find(name);
    if (it == section_indices.end()) {
        std::size_t idx = sections.size();
        section_indices[name] = idx;
        sections.push_back({name, 0.0, 0, 1e100, 0.0, 0.0});
        current_section_ = idx;
    } else {
        current_section_ = it->second;
    }
    section_start_ = std::chrono::high_resolution_clock::now();
}

void RendererDebug::ProfilerData::end_section(const std::string& name) {
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(end - section_start_).count();

    auto it = section_indices.find(name);
    if (it == section_indices.end()) return;

    std::size_t idx = it->second;
    auto& section = sections[idx];

    section.call_count++;
    section.time_ms += elapsed;
    section.min_time_ms = std::min(section.min_time_ms, elapsed);
    section.max_time_ms = std::max(section.max_time_ms, elapsed);
    section.avg_time_ms = section.time_ms / section.call_count;
}

void RendererDebug::ProfilerData::reset() {
    for (auto& section : sections) {
        section.time_ms = 0.0;
        section.call_count = 0;
        section.min_time_ms = 1e100;
        section.max_time_ms = 0.0;
        section.avg_time_ms = 0.0;
    }
}

std::string RendererDebug::ProfilerData::report() const {
    std::ostringstream oss;
    oss << "=== Profiler Report ===\n";

    for (const auto& section : sections) {
        if (section.call_count == 0) continue;
        oss << std::setw(20) << section.name << ": "
           << std::fixed << std::setprecision(2) << section.time_ms << " ms "
           << "(" << section.call_count << " calls, avg: " << section.avg_time_ms << " ms)\n";
    }

    return oss.str();
}

std::string RendererDebug::ProfilerData::report_json() const {
    std::ostringstream oss;
    oss << "{";

    for (std::size_t i = 0; i < sections.size(); ++i) {
        const auto& section = sections[i];
        if (section.call_count == 0) continue;

        if (i > 0) oss << ",";
        oss << "\"" << section.name << "\":{"
           << "\"time_ms\":" << section.time_ms << ","
           << "\"call_count\":" << section.call_count << ","
           << "\"avg_time_ms\":" << section.avg_time_ms << "}";
    }

    oss << "}";
    return oss.str();
}

void RendererDebug::set_diagnostics(const Diagnostics& diag) {
    diagnostics_ = diag;
}

std::string RendererDebug::diagnostics_report() const {
    std::ostringstream oss;
    oss << "=== Diagnostics ===\n";
    oss << "Triangle count:    " << diagnostics_.triangles_rendered << "\n";
    oss << "Rays traced:       " << diagnostics_.rays_traced << "\n";
    oss << "Rays/pixel:       " << diagnostics_.rays_per_pixel << "\n";
    oss << "Visible objects:  " << diagnostics_.visible_objects << "\n";
    oss << "Culled objects:   " << diagnostics_.culled_objects << "\n";
    oss << "Memory:          " << format_memory(diagnostics_.memory_used_mb) << "\n";
    oss << "Frame time:       " << format_time(diagnostics_.frame_time_ms) << "\n";
    oss << "Render time:      " << format_time(diagnostics_.render_time_ms) << "\n";
    oss << "FPS:             " << std::fixed << std::setprecision(1) << diagnostics_.fps << "\n";

    if (diagnostics_.cache_hits + diagnostics_.cache_misses > 0) {
        float hit_rate = 100.0f * diagnostics_.cache_hits / (diagnostics_.cache_hits + diagnostics_.cache_misses);
        oss << "Cache hit rate:   " << format_percentage(hit_rate) << "\n";
    }

    return oss.str();
}

std::string RendererDebug::format_memory(double mb) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << mb << " MB";
    return oss.str();
}

std::string RendererDebug::format_time(double ms) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << ms << " ms";
    return oss.str();
}

std::string RendererDebug::format_percentage(float value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << value << "%";
    return oss.str();
}

std::vector<SceneInspector::LayerInfo> SceneInspector::inspect_layers(const scene::EvaluatedCompositionState& state) {
    std::vector<LayerInfo> layers;

    for (const auto& layer : state.layers) {
        LayerInfo info;
        info.id = layer.id;
        info.name = layer.name;
        info.type = layer.type;
        info.visible = layer.visible;
        info.is_3d = layer.is_3d;
        info.world_position = layer.world_position3;
        info.world_matrix = layer.world_matrix;
        info.bounds = SceneCulling::BoundingBox::from_center_extents(layer.world_position3, layer.scale_3d);

        layers.push_back(info);
    }

    return layers;
}

std::vector<SceneInspector::LightInfo> SceneInspector::inspect_lights(const scene::EvaluatedCompositionState& state) {
    std::vector<LightInfo> lights;

    for (const auto& light : state.lights) {
        LightInfo info;
        info.id = light.layer_id;
        info.type = light.type;
        info.position = light.position;
        info.direction = light.direction;
        info.intensity = light.intensity;
        info.attenuation_near = light.attenuation_near;
        info.attenuation_far = light.attenuation_far;

        lights.push_back(info);
    }

    return lights;
}

SceneInspector::CameraInfo SceneInspector::inspect_camera(const scene::EvaluatedCameraState& state) {
    CameraInfo info;
    if (!state.available) return info;

    info.id = state.layer_id;
    info.position = state.position;
    info.point_of_interest = state.point_of_interest;
    info.fov = state.camera.fov_y_rad * 180.0f / 3.14159f;
    info.aperture = state.aperture;
    info.focus_distance = state.focus_distance;

    return info;
}

SceneInspector::MemoryInfo SceneInspector::estimate_memory(const scene::EvaluatedCompositionState& state, const render::RenderIntent* intent) {
    MemoryInfo info;

    for (const auto& layer : state.layers) {
        const render::RenderIntent::LayerResources* res = nullptr;
        if (intent) {
            auto it = intent->layer_resources.find(layer.id);
            if (it != intent->layer_resources.end()) {
                res = &it->second;
            }
        }

        if (res && res->mesh_asset) {
            for (const auto& sub : res->mesh_asset->sub_meshes) {
                info.geometry_bytes += sub.vertices.size() * sizeof(media::MeshAsset::Vertex);
                info.geometry_bytes += sub.indices.size() * sizeof(unsigned int);
            }
        }

        if (res && res->texture_rgba && layer.width > 0 && layer.height > 0) {
            info.texture_bytes += static_cast<std::size_t>(layer.width) * layer.height * 4;
        }
    }

    info.total_bytes = info.geometry_bytes + info.texture_bytes + info.buffer_bytes;
    return info;
}

std::string SceneInspector::layer_report(const std::vector<LayerInfo>& layers) {
    std::ostringstream oss;
    oss << "=== Layer Report ===\n";

    for (const auto& layer : layers) {
        oss << "Layer: " << layer.name << " (" << layer.id << ")\n";
        oss << "  Type: " << static_cast<int>(layer.type) << ", Visible: " << (layer.visible ? "yes" : "no") << ", 3D: " << (layer.is_3d ? "yes" : "no") << "\n";
        oss << "  World Pos: (" << layer.world_position.x << ", " << layer.world_position.y << ", " << layer.world_position.z << ")\n";
    }

    return oss.str();
}

std::string SceneInspector::light_report(const std::vector<LightInfo>& lights) {
    std::ostringstream oss;
    oss << "=== Light Report ===\n";

    for (const auto& light : lights) {
        oss << "Light: " << light.id << " (" << light.type << ")\n";
        oss << "  Position: (" << light.position.x << ", " << light.position.y << ", " << light.position.z << ")\n";
        oss << "  Intensity: " << light.intensity << "\n";
    }

    return oss.str();
}

std::string SceneInspector::camera_report(const CameraInfo& camera) {
    std::ostringstream oss;
    oss << "=== Camera Report ===\n";
    oss << "ID: " << camera.id << "\n";
    oss << "Position: (" << camera.position.x << ", " << camera.position.y << ", " << camera.position.z << ")\n";
    oss << "POI: (" << camera.point_of_interest.x << ", " << camera.point_of_interest.y << ", " << camera.point_of_interest.z << ")\n";
    oss << "FOV: " << camera.fov << " deg\n";
    oss << "Aperture: " << camera.aperture << "\n";
    oss << "Focus Distance: " << camera.focus_distance << "\n";

    return oss.str();
}

std::string SceneInspector::memory_report(const MemoryInfo& memory) {
    std::ostringstream oss;
    oss << "=== Memory Report ===\n";
    oss << "Geometry: " << RendererDebug::format_memory(memory.geometry_bytes / (1024.0 * 1024.0)) << "\n";
    oss << "Textures:  " << RendererDebug::format_memory(memory.texture_bytes / (1024.0 * 1024.0)) << "\n";
    oss << "Buffers:   " << RendererDebug::format_memory(memory.buffer_bytes / (1024.0 * 1024.0)) << "\n";
    oss << "Total:    " << RendererDebug::format_memory(memory.total_bytes / (1024.0 * 1024.0)) << "\n";

    return oss.str();
}

} // namespace tachyon::renderer3d
