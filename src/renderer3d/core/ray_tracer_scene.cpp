#include "tachyon/renderer3d/core/ray_tracer.h"
#include "tachyon/media/processing/extruder.h"
#include "tachyon/text/rendering/outline_extractor.h"
#include "tachyon/text/layout/layout.h"
#include "tachyon/text/fonts/font_registry.h"
#include "tachyon/core/math/matrix4x4.h"
#include <algorithm>
#include <cstring>

namespace tachyon::renderer3d {

void RayTracer::cleanup_scene() {
    if (scene_) { rtcReleaseScene(scene_); scene_ = nullptr; }
    instances_.clear(); extruded_assets_.clear();
}

void RayTracer::build_scene(const scene::EvaluatedCompositionState& state, const text::Font* font, const text::FontRegistry* registry) {
    cleanup_scene();
    if (!device_) return;
    scene_ = rtcNewScene(device_);
    const media::HDRTextureData* new_env = reinterpret_cast<const media::HDRTextureData*>(state.environment_map);
    if (new_env != environment_map_) { environment_manager_.update_prefiltered_env(new_env); environment_map_ = new_env; }
    environment_intensity_ = state.environment_intensity; environment_rotation_ = state.environment_intensity_rotation;
    environment_manager_.ensure_brdf_lut();
    internal_build_scene(state, [](std::size_t) { return true; }, font, registry);
}

void RayTracer::build_scene_subset(const scene::EvaluatedCompositionState& state, const std::vector<std::size_t>& layer_indices, const text::Font* font, const text::FontRegistry* registry) {
    cleanup_scene();
    if (!device_) return;
    scene_ = rtcNewScene(device_);
    const media::HDRTextureData* new_env = reinterpret_cast<const media::HDRTextureData*>(state.environment_map);
    if (new_env != environment_map_) { environment_manager_.update_prefiltered_env(new_env); environment_map_ = new_env; }
    environment_intensity_ = state.environment_intensity; environment_rotation_ = state.environment_intensity_rotation;
    environment_manager_.ensure_brdf_lut();
    internal_build_scene(state, [&](std::size_t idx) { return std::find(layer_indices.begin(), layer_indices.end(), idx) != layer_indices.end(); }, font, registry);
}

void RayTracer::internal_build_scene(const scene::EvaluatedCompositionState& state, const std::function<bool(std::size_t)>& filter, const text::Font* font, const text::FontRegistry* registry) {
    for (std::size_t layer_idx = 0; layer_idx < state.layers.size(); ++layer_idx) {
        const auto& layer = state.layers[layer_idx];
        if (!filter(layer_idx) || !layer.is_3d || !layer.visible) continue;

        RTCScene sub_scene = nullptr;
        const auto* media_mesh = reinterpret_cast<const media::MeshAsset*>(layer.mesh_asset);
        if (media_mesh && !media_mesh->empty()) {
            const std::uint32_t asset_key = (std::uint32_t)(std::uintptr_t)media_mesh;
            auto it = mesh_cache_.find(asset_key);
            if (it != mesh_cache_.end()) sub_scene = it->second.scene;
            else {
                sub_scene = rtcNewScene(device_);
                for (const auto& sub : media_mesh->sub_meshes) {
                    RTCGeometry geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);
                    float* v = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), sub.vertices.size());
                    for (size_t i = 0; i < sub.vertices.size(); ++i) { math::Vector3 p = sub.transform.transform_point(sub.vertices[i].position); v[i*3+0] = p.x; v[i*3+1] = p.y; v[i*3+2] = p.z; }
                    unsigned int* idx = (unsigned int*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), sub.indices.size() / 3);
                    std::memcpy(idx, sub.indices.data(), sub.indices.size() * sizeof(unsigned int));
                    rtcCommitGeometry(geom); rtcAttachGeometry(sub_scene, geom); rtcReleaseGeometry(geom);
                }
                rtcCommitScene(sub_scene); mesh_cache_[asset_key] = {sub_scene};
            }
            RTCGeometry inst = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_INSTANCE); rtcSetGeometryInstancedScene(inst, sub_scene);
            const auto& m = layer.world_matrix; float t[12] = { m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10], m[12], m[13], m[14] };
            rtcSetGeometryTransform(inst, 0, RTC_FORMAT_FLOAT3X4_ROW_MAJOR, t); rtcCommitGeometry(inst);
            instances_.push_back({rtcAttachGeometry(scene_, inst), &layer, media_mesh, nullptr}); rtcReleaseGeometry(inst);
        } else if (layer.type == scene::LayerType::Text && !layer.text_content.empty()) {
            text::TextStyle style; style.pixel_size = (uint32_t)layer.font_size;
            text::TextBox box{ 10000, 10000, true }; 
            text::TextAlignment align = (layer.text_alignment == 1) ? text::TextAlignment::Center : (layer.text_alignment == 2 ? text::TextAlignment::Right : text::TextAlignment::Left);
            
            text::TextLayoutResult layout;
            if (registry) {
                layout = text::layout_text(*registry, layer.font_id, layer.text_content, style, box, align);
            } else if (font) {
                layout = text::layout_text(*font, layer.text_content, style, box, align);
            } else {
                continue;
            }

            for (const auto& glyph : layout.glyphs) {
                const text::Font* actual_font = nullptr;
                if (registry) {
                    actual_font = registry->find_by_id(glyph.font_id);
                } else {
                    actual_font = font;
                }
                
                if (!actual_font) continue;

                auto contours = text::OutlineExtractor::extract_glyph_outline(*actual_font, glyph.font_glyph_index, 1.0f);
                for (const auto& contour : contours) {
                    RTCScene gs = rtcNewScene(device_); auto asset = std::make_unique<media::MeshAsset>(); asset->sub_meshes.push_back(media::Extruder::extrude_shape(contour, layer.extrusion_depth, layer.bevel_size));
                    const auto& sub = asset->sub_meshes[0]; RTCGeometry geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);
                    float* v = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), sub.vertices.size());
                    for (size_t i = 0; i < sub.vertices.size(); ++i) { v[i*3+0] = sub.vertices[i].position.x + glyph.x; v[i*3+1] = sub.vertices[i].position.y + glyph.y; v[i*3+2] = sub.vertices[i].position.z; }
                    unsigned int* idx = (unsigned int*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), sub.indices.size() / 3);
                    std::memcpy(idx, sub.indices.data(), sub.indices.size() * sizeof(unsigned int));
                    rtcCommitGeometry(geom); rtcAttachGeometry(gs, geom); rtcReleaseGeometry(geom); rtcCommitScene(gs);
                    RTCGeometry inst = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_INSTANCE); rtcSetGeometryInstancedScene(inst, gs);
                    const auto& m = layer.world_matrix; float t[12] = { m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10], m[12], m[13], m[14] };
                    rtcSetGeometryTransform(inst, 0, RTC_FORMAT_FLOAT3X4_ROW_MAJOR, t); rtcCommitGeometry(inst);
                    instances_.push_back({rtcAttachGeometry(scene_, inst), &layer, nullptr, nullptr}); rtcReleaseGeometry(inst); rtcReleaseScene(gs); extruded_assets_.push_back(std::move(asset));
                }
            }
        } else if (layer.type == scene::LayerType::Shape && layer.shape_path.has_value()) {
            RTCScene ts = rtcNewScene(device_); auto asset = std::make_unique<media::MeshAsset>(); asset->sub_meshes.push_back(media::Extruder::extrude_shape(*layer.shape_path, layer.extrusion_depth, layer.bevel_size));
            const auto& sub = asset->sub_meshes[0]; RTCGeometry geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);
            float* v = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), sub.vertices.size());
            for (size_t i = 0; i < sub.vertices.size(); ++i) { v[i*3+0] = sub.vertices[i].position.x; v[i*3+1] = sub.vertices[i].position.y; v[i*3+2] = sub.vertices[i].position.z; }
            unsigned int* idx = (unsigned int*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), sub.indices.size() / 3);
            std::memcpy(idx, sub.indices.data(), sub.indices.size() * sizeof(unsigned int));
            rtcCommitGeometry(geom); rtcAttachGeometry(ts, geom); rtcReleaseGeometry(geom); rtcCommitScene(ts);
            RTCGeometry inst = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_INSTANCE); rtcSetGeometryInstancedScene(inst, ts);
            const auto& m = layer.world_matrix; float t[12] = { m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10], m[12], m[13], m[14] };
            rtcSetGeometryTransform(inst, 0, RTC_FORMAT_FLOAT3X4_ROW_MAJOR, t); rtcCommitGeometry(inst);
            instances_.push_back({rtcAttachGeometry(scene_, inst), &layer, nullptr, nullptr}); rtcReleaseGeometry(inst); rtcReleaseScene(ts); extruded_assets_.push_back(std::move(asset));
        } else {
            const std::uint32_t pk = 0xFFFFFFFF; auto it = mesh_cache_.find(pk); if (it != mesh_cache_.end()) sub_scene = it->second.scene;
            else {
                sub_scene = rtcNewScene(device_); RTCGeometry geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);
                float* v = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), 4);
                v[0] = -1.0f; v[1] = -1.0f; v[2] = 0.0f; v[3] = 1.0f; v[4] = -1.0f; v[5] = 0.0f; v[6] = 1.0f; v[7] = 1.0f; v[8] = 0.0f; v[9] = -1.0f; v[10] = 1.0f; v[11] = 0.0f;
                unsigned int* idx = (unsigned int*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), 2);
                idx[0] = 0; idx[1] = 1; idx[2] = 2; idx[3] = 0; idx[4] = 2; idx[5] = 3;
                rtcCommitGeometry(geom); rtcAttachGeometry(sub_scene, geom); rtcReleaseGeometry(geom); rtcCommitScene(sub_scene); mesh_cache_[pk] = {sub_scene};
            }
            RTCGeometry inst = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_INSTANCE); rtcSetGeometryInstancedScene(inst, sub_scene);
            const auto& m = layer.world_matrix; float t[12] = { m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10], m[12], m[13], m[14] };
            rtcSetGeometryTransform(inst, 0, RTC_FORMAT_FLOAT3X4_ROW_MAJOR, t); rtcCommitGeometry(inst);
            instances_.push_back({rtcAttachGeometry(scene_, inst), &layer, nullptr, nullptr}); rtcReleaseGeometry(inst);
        }
    }
}

} // namespace tachyon::renderer3d
