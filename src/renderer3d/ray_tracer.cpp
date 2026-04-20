#ifdef _MSC_VER
#pragma warning(disable: 4324)
#endif

#include "tachyon/renderer3d/ray_tracer.h"
#include "tachyon/media/extruder.h"
#include "tachyon/text/outline_extractor.h"
#include "tachyon/text/layout.h"
#include "tachyon/renderer2d/texture_resolver.h"
#include "tachyon/core/math/matrix4x4.h"
#include <OpenImageDenoise/oidn.hpp>
#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>

namespace tachyon {
namespace renderer3d {

std::unique_ptr<media::BRDFLut> RayTracer::brdf_lut_ = nullptr;

const float PI = 3.1415926535f;

namespace {
// PBR Helpers
math::Vector3 fresnel_schlick(float cos_theta, const math::Vector3& f0) {
    return f0 + (math::Vector3{1.0f, 1.0f, 1.0f} - f0) * std::pow(std::clamp(1.0f - cos_theta, 0.0f, 1.0f), 5.0f);
}

float ndf_ggx(float n_dot_h, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float n_dot_h2 = n_dot_h * n_dot_h;
    float denom = (n_dot_h2 * (a2 - 1.0f) + 1.0f);
    return a2 / (PI * denom * denom);
}

float geometry_schlick_ggx(float n_dot_v, float roughness) {
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    return n_dot_v / (n_dot_v * (1.0f - k) + k);
}

float geometry_smith(float n_dot_v, float n_dot_l, float roughness) {
    return geometry_schlick_ggx(n_dot_v, roughness) * geometry_schlick_ggx(n_dot_l, roughness);
}

// Simple random sampling on a disk for DOF and soft shadows
math::Vector2 sample_disk(float radius, float u1, float u2) {
    if (radius <= 0.0f) return {0.0f, 0.0f};
    float r = radius * std::sqrt(u1);
    float theta = 2.0f * PI * u2;
    return { r * std::cos(theta), r * std::sin(theta) };
}

// GGX Importance Sampling
math::Vector3 importance_sample_ggx(float u1, float u2, const math::Vector3& N, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0f * PI * u1;
    float cos_theta = std::sqrt((1.0f - u2) / (1.0f + (a * a - 1.0f) * u2));
    float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);

    math::Vector3 H;
    H.x = sin_theta * std::cos(phi);
    H.y = sin_theta * std::sin(phi);
    H.z = cos_theta;

    math::Vector3 up = std::abs(N.z) < 0.999f ? math::Vector3{0, 0, 1} : math::Vector3{1, 0, 0};
    math::Vector3 tangent = math::Vector3::cross(up, N).normalized();
    math::Vector3 bitangent = math::Vector3::cross(N, tangent);

    return (tangent * H.x + bitangent * H.y + N * H.z).normalized();
}

// Equirectangular HDR Sampling
math::Vector3 sample_equirect(const media::HDRTextureData& map, const math::Vector3& dir, float rotation) {
    float phi = std::atan2(dir.z, dir.x) + rotation * (PI / 180.0f);
    float theta = std::asin(std::clamp(dir.y, -1.0f, 1.0f));
    float u = 0.5f + phi / (2.0f * PI);
    float v = 0.5f - theta / PI;
    
    // Simple bilinear or point sampling
    int tx = std::clamp(static_cast<int>(u * map.width), 0, map.width - 1);
    int ty = std::clamp(static_cast<int>(v * map.height), 0, map.height - 1);
    int idx = (ty * map.width + tx) * map.channels;
    
    if (map.channels >= 3) {
        return { map.data[idx+0], map.data[idx+1], map.data[idx+2] };
    }
    return { 0, 0, 0 };
}
}

void RayTracer::log_embree_error(void* userPtr, RTCError code, const char* str) {
    (void)userPtr;
    if (code == RTC_ERROR_NONE) return;
    std::cerr << "Embree Error [" << code << "]: " << str << std::endl;
}

RayTracer::RayTracer() {
    device_ = rtcNewDevice(nullptr);
    if (!device_) {
        return;
    }
    rtcSetDeviceErrorFunction(device_, log_embree_error, nullptr);

    oidn_device_ = oidn::newDevice();
    oidn_device_.commit();
    oidn_filter_ = oidn_device_.newFilter("RT");
}

RayTracer::~RayTracer() {
    cleanup_scene();
    if (device_) {
        rtcReleaseDevice(device_);
    }
}

void RayTracer::cleanup_scene() {
    if (scene_) {
        rtcReleaseScene(scene_);
        scene_ = nullptr;
    }
    instances_.clear();
    extruded_assets_.clear();
}

void RayTracer::build_scene(const scene::EvaluatedCompositionState& state) {
    cleanup_scene();
    if (!device_) return;

    scene_ = rtcNewScene(device_);

    // Initialize environment properties from state
    const media::HDRTextureData* new_env = reinterpret_cast<const media::HDRTextureData*>(state.environment_map);
    if (new_env != environment_map_) {
        update_prefiltered_env(new_env);
        environment_map_ = new_env;
    }
    environment_intensity_ = state.environment_intensity;
    environment_rotation_ = state.environment_rotation;
    ensure_brdf_lut();

    auto should_include = [&](std::size_t idx) { return true; };
    internal_build_scene(state, should_include);
}

void RayTracer::build_scene_subset(const scene::EvaluatedCompositionState& state, const std::vector<std::size_t>& layer_indices) {
    cleanup_scene();
    if (!device_) return;

    scene_ = rtcNewScene(device_);

    // Initialize environment properties from state
    const media::HDRTextureData* new_env = reinterpret_cast<const media::HDRTextureData*>(state.environment_map);
    if (new_env != environment_map_) {
        update_prefiltered_env(new_env);
        environment_map_ = new_env;
    }
    environment_intensity_ = state.environment_intensity;
    environment_rotation_ = state.environment_rotation;
    ensure_brdf_lut();

    auto should_include = [&](std::size_t idx) {
        return std::find(layer_indices.begin(), layer_indices.end(), idx) != layer_indices.end();
    };
    internal_build_scene(state, should_include);
}

void RayTracer::internal_build_scene(const scene::EvaluatedCompositionState& state, const std::function<bool(std::size_t)>& filter) {
    for (std::size_t layer_idx = 0; layer_idx < state.layers.size(); ++layer_idx) {
        const auto& layer = state.layers[layer_idx];
        if (!filter(layer_idx)) continue;

        if (!layer.is_3d || !layer.visible) continue;
        const auto* media_mesh = reinterpret_cast<const media::MeshAsset*>(layer.mesh_asset);
        if (media_mesh && !media_mesh->empty()) {
            const auto& asset = *media_mesh;
            for (const auto& sub : asset.sub_meshes) {
                RTCGeometry geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);

                float* vertices = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), sub.vertices.size());
                for (std::size_t i = 0; i < sub.vertices.size(); ++i) {
                    math::Vector3 p = sub.vertices[i].position;
                    math::Vector3 n = sub.vertices[i].normal;
                    
                    // 1. Apply Morphing
                    if (!layer.morph_weights.empty() && !sub.morph_targets.empty()) {
                        for (size_t m = 0; m < std::min(layer.morph_weights.size(), sub.morph_targets.size()); ++m) {
                            float w = layer.morph_weights[m];
                            if (w > 0.001f && i < sub.morph_targets[m].positions.size()) {
                                p += sub.morph_targets[m].positions[i] * w;
                            }
                        }
                    }
                    
                    // 2. Apply Skinning
                    if (!layer.joint_matrices.empty()) {
                        math::Vector3 skinned_p{0,0,0};
                        math::Vector3 skinned_n{0,0,0};
                        for (int k = 0; k < 4; ++k) {
                            float w = sub.vertices[i].weights[k];
                            if (w > 0.001f) {
                                uint32_t j_idx = sub.vertices[i].joints[k];
                                if (j_idx < layer.joint_matrices.size()) {
                                    skinned_p += layer.joint_matrices[j_idx].transform_point(p) * w;
                                    skinned_n += layer.joint_matrices[j_idx].transform_vector(n) * w;
                                }
                            }
                        }
                        if (sub.vertices[i].weights[0] > 0.001f) {
                            p = skinned_p;
                            n = skinned_n.normalized();
                        }
                    }
                    
                    // Combine glTF node local transform with the layer's 3D world transform
                    math::Vector3 tp = sub.transform.transform_point(p);
                    math::Vector3 wp = layer.world_matrix.transform_point(tp);
                    
                    vertices[i * 3 + 0] = wp.x;
                    vertices[i * 3 + 1] = wp.y;
                    vertices[i * 3 + 2] = wp.z;
                }

                unsigned int* indices = (unsigned int*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), sub.indices.size() / 3);
                std::memcpy(indices, sub.indices.data(), sub.indices.size() * sizeof(unsigned int));

                rtcCommitGeometry(geom);
                unsigned int geom_id = rtcAttachGeometry(scene_, geom);
                rtcReleaseGeometry(geom);

                instances_.push_back({geom_id, &layer, &asset, &sub});
            }
        } else if (layer.type == scene::LayerType::Shape && layer.shape_path.has_value() && layer.extrusion_depth > 0.01f) {
            auto asset = std::make_unique<media::MeshAsset>();
            asset->sub_meshes.push_back(media::Extruder::extrude_shape(*layer.shape_path, layer.extrusion_depth, layer.bevel_size));
            const auto& asset_ref = *asset;
            const auto& sub = asset_ref.sub_meshes[0];
            
            RTCGeometry geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);
            float* vertices = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), sub.vertices.size());
            for (std::size_t i = 0; i < sub.vertices.size(); ++i) {
                math::Vector3 p = sub.vertices[i].position;
                math::Vector3 wp = layer.world_matrix.transform_point(p);
                vertices[i * 3 + 0] = wp.x;
                vertices[i * 3 + 1] = wp.y;
                vertices[i * 3 + 2] = wp.z;
            }

            unsigned int* indices = (unsigned int*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), sub.indices.size() / 3);
            std::memcpy(indices, sub.indices.data(), sub.indices.size() * sizeof(unsigned int));

            rtcCommitGeometry(geom);
            unsigned int geom_id = rtcAttachGeometry(scene_, geom);
            rtcReleaseGeometry(geom);

            instances_.push_back({geom_id, &layer, &asset_ref, &sub});
            extruded_assets_.push_back(std::move(asset));
        } else if (layer.type == scene::LayerType::Text && !layer.text_content.empty() && layer.extrusion_depth > 0.01f) {
            const auto* font = renderer2d::TextRenderConfig::instance().font();
            if (font) {
                text::TextStyle style;
                style.pixel_size = static_cast<std::uint32_t>(layer.font_size);
                
                text::TextBox box;
                box.width = 10000;
                box.height = 10000;
                
                text::TextAlignment align = text::TextAlignment::Left;
                if (layer.text_alignment == 1) align = text::TextAlignment::Center;
                else if (layer.text_alignment == 2) align = text::TextAlignment::Right;
                
                auto layout = text::layout_text(*font, layer.text_content, style, box, align);
                
                for (const auto& glyph : layout.glyphs) {
                    auto contours = text::OutlineExtractor::extract_glyph_outline(*font, glyph.font_glyph_index, 1.0f);
                    
                    // Position glyph relative to layer center (approximate)
                    math::Vector3 glyph_offset = { static_cast<float>(glyph.x), static_cast<float>(glyph.y), 0.0f };

                    for (const auto& contour : contours) {
                        auto asset = std::make_unique<media::MeshAsset>();
                        asset->sub_meshes.push_back(media::Extruder::extrude_shape(contour, layer.extrusion_depth, layer.bevel_size));
                        const auto& asset_ref = *asset;
                        const auto& sub = asset_ref.sub_meshes[0];
                        
                        RTCGeometry geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);
                        float* vertices = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), sub.vertices.size());
                        for (std::size_t i = 0; i < sub.vertices.size(); ++i) {
                            math::Vector3 p = sub.vertices[i].position + glyph_offset;
                            math::Vector3 wp = layer.world_matrix.transform_point(p);
                            vertices[i * 3 + 0] = wp.x;
                            vertices[i * 3 + 1] = wp.y;
                            vertices[i * 3 + 2] = wp.z;
                        }

                        unsigned int* indices = (unsigned int*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), sub.indices.size() / 3);
                        std::memcpy(indices, sub.indices.data(), sub.indices.size() * sizeof(unsigned int));

                        rtcCommitGeometry(geom);
                        unsigned int geom_id = rtcAttachGeometry(scene_, geom);
                        rtcReleaseGeometry(geom);

                        instances_.push_back({geom_id, &layer, &asset_ref, &sub});
                        extruded_assets_.push_back(std::move(asset));
                    }
                }
            }
        } else {
            RTCGeometry geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);
            float* vertices = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), 4);
            
            float hw = static_cast<float>(layer.width) * 0.5f;
            float hh = static_cast<float>(layer.height) * 0.5f;
            
            auto transform_pt = [&](float x, float y) {
                return layer.world_matrix.transform_point({x, y, 0.0f});
            };

            math::Vector3 p0 = transform_pt(-hw, -hh);
            math::Vector3 p1 = transform_pt(hw, -hh);
            math::Vector3 p2 = transform_pt(hw, hh);
            math::Vector3 p3 = transform_pt(-hw, hh);

            vertices[0] = p0.x; vertices[1] = p0.y; vertices[2] = p0.z;
            vertices[3] = p1.x; vertices[4] = p1.y; vertices[5] = p1.z;
            vertices[6] = p2.x; vertices[7] = p2.y; vertices[8] = p2.z;
            vertices[9] = p3.x; vertices[10] = p3.y; vertices[11] = p3.z;

            unsigned int* indices = (unsigned int*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), 2);
            indices[0] = 0; indices[1] = 1; indices[2] = 2;
            indices[3] = 0; indices[4] = 2; indices[5] = 3;

            rtcCommitGeometry(geom);
            unsigned int geom_id = rtcAttachGeometry(scene_, geom);
            rtcReleaseGeometry(geom);

            instances_.push_back({geom_id, &layer, nullptr});
        }
    }

    rtcCommitScene(scene_);
}

RayTracer::ShadingResult RayTracer::sample_environment(const math::Vector3& direction) {
    return sample_environment(direction, 0.0f);
}

RayTracer::ShadingResult RayTracer::sample_environment(const math::Vector3& direction, float roughness) {
    if (environment_map_) {
        if (prefiltered_env_ && !prefiltered_env_->levels.empty()) {
            // Roughness 0..1 maps to levels 0..N-1
            float level = roughness * (prefiltered_env_->levels.size() - 1);
            int idx0 = static_cast<int>(std::floor(level));
            int idx1 = std::min(idx0 + 1, static_cast<int>(prefiltered_env_->levels.size() - 1));
            float f = level - idx0;
            
            const auto& l0 = prefiltered_env_->levels[idx0];
            const auto& l1 = prefiltered_env_->levels[idx1];
            
            auto s0 = sample_equirect({l0.data, l0.width, l0.height, 3}, direction, environment_rotation_);
            auto s1 = sample_equirect({l1.data, l1.width, l1.height, 3}, direction, environment_rotation_);
            
            return { (s0 * (1.0f - f) + s1 * f) * environment_intensity_, 0.0f };
        }
        return { sample_equirect(*environment_map_, direction, environment_rotation_) * environment_intensity_, 0.0f };
    }

    // Simple vertical gradient sky fallback
    float t = 0.5f * (direction.y + 1.0f);
    // Blueish sky
    math::Vector3 sky_top = {0.2f, 0.5f, 1.0f};
    math::Vector3 sky_bottom = {0.8f, 0.9f, 1.0f};
    return {sky_bottom * (1.0f - t) + sky_top * t, 0.0f}; // alpha = 0.0 for environment
}

void RayTracer::ensure_brdf_lut() {
    if (brdf_lut_) return;
    
    const int size = 512;
    brdf_lut_ = std::make_unique<media::BRDFLut>();
    brdf_lut_->size = size;
    brdf_lut_->data.resize(size * size * 2);
    
    const int num_samples = 1024;
    #pragma omp parallel for
    for (int y = 0; y < size; ++y) {
        float n_dot_v = (static_cast<float>(y) + 0.5f) / size;
        for (int x = 0; x < size; ++x) {
            float roughness = (static_cast<float>(x) + 0.5f) / size;
            
            // Numerical integration for Split-Sum BRDF
            float a = 0.0f;
            float b = 0.0f;
            
            math::Vector3 v;
            v.x = std::sqrt(1.0f - n_dot_v * n_dot_v);
            v.y = 0.0f;
            v.z = n_dot_v;

            for (int i = 0; i < num_samples; ++i) {
                // Hammersley sequence or similar for importance sampling
                // For a "fix veloce", we'll use a slightly better approximation 
                // but still robust.
                float u1 = static_cast<float>(i) / num_samples;
                float u2 = static_cast<float>(i % 127) / 127.0f; // pseudo-random
                
                // GGX Importance Sampling
                float alpha = roughness * roughness;
                float phi = 2.0f * PI * u1;
                float cos_theta = std::sqrt((1.0f - u2) / (1.0f + (alpha * alpha - 1.0f) * u2));
                float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);
                
                math::Vector3 h;
                h.x = sin_theta * std::cos(phi);
                h.y = sin_theta * std::sin(phi);
                h.z = cos_theta;
                
                math::Vector3 l = (h * 2.0f * math::Vector3::dot(v, h) - v).normalized();
                
                float n_dot_l = std::max(l.z, 0.0001f);
                float n_dot_h = std::max(h.z, 0.0001f);
                float v_dot_h = std::max(math::Vector3::dot(v, h), 0.0001f);
                
                if (n_dot_l > 0.0f) {
                    // Geometry function (G_Smith)
                    float k = (roughness * roughness) / 2.0f;
                    auto g_v = [&](float ndv) { return ndv / (ndv * (1.0f - k) + k); };
                    float g = g_v(n_dot_v) * g_v(n_dot_l);
                    float g_vis = (g * v_dot_h) / (n_dot_h * n_dot_v);
                    float fc = std::pow(1.0f - v_dot_h, 5.0f);
                    
                    a += (1.0f - fc) * g_vis;
                    b += fc * g_vis;
                }
            }
            
            brdf_lut_->data[(y * size + x) * 2 + 0] = a / num_samples;
            brdf_lut_->data[(y * size + x) * 2 + 1] = b / num_samples;
        }
    }
}

void RayTracer::update_prefiltered_env(const media::HDRTextureData* map) {
    if (!map) {
        prefiltered_env_.reset();
        return;
    }
    
    prefiltered_env_ = std::make_unique<media::PreFilteredEnvMap>();
    const int num_levels = 5;
    
    for (int i = 0; i < num_levels; ++i) {
        float roughness = static_cast<float>(i) / (num_levels - 1);
        int w = std::max(1, map->width >> i);
        int h = std::max(1, map->height >> i);
        
        media::PreFilteredEnvMap::Level level;
        level.width = w;
        level.height = h;
        level.data.resize(w * h * 3);
        
        // Very crude downsampling / box blur to simulate roughness
        // Real pre-filtering requires importance sampling the environment map
        // but for a CPU renderer we can start with this or implement a faster convolution
        #pragma omp parallel for
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                // For now, just copy or simple box filter
                // TODO: Implement proper GGX convolution for environment map
                float u = static_cast<float>(x) / w;
                float v = static_cast<float>(y) / h;
                int sx = std::clamp(static_cast<int>(u * map->width), 0, map->width - 1);
                int sy = std::clamp(static_cast<int>(v * map->height), 0, map->height - 1);
                int s_idx = (sy * map->width + sx) * map->channels;
                int d_idx = (y * w + x) * 3;
                level.data[d_idx+0] = map->data[s_idx+0];
                level.data[d_idx+1] = map->data[s_idx+1];
                level.data[d_idx+2] = map->data[s_idx+2];
            }
        }
        prefiltered_env_->levels.push_back(std::move(level));
    }
}

RayTracer::ShadingResult RayTracer::trace_ray(
    const math::Vector3& origin,
    const math::Vector3& direction,
    const scene::EvaluatedCompositionState& state,
    std::mt19937& rng,
    int depth) 
{
    if (depth > kMaxBounces) return {{0,0,0}, 0.0f};

    RTCRayHit rh{};
    rh.ray.org_x = origin.x; rh.ray.org_y = origin.y; rh.ray.org_z = origin.z;
    rh.ray.dir_x = direction.x; rh.ray.dir_y = direction.y; rh.ray.dir_z = direction.z;
    rh.ray.tnear = 0.001f; // Avoid self-intersection
    rh.ray.tfar = 100000.0f;
    rh.ray.mask = static_cast<unsigned int>(-1);
    rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    RTCIntersectArguments args;
    rtcInitIntersectArguments(&args);
    rtcIntersect1(scene_, &rh, &args);

    if (rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        auto env = sample_environment(direction);
        return { env.color, env.alpha, 1e10f }; // Infinite depth for sky
    }

    const scene::EvaluatedLayerState* layer = nullptr;
    const media::MeshAsset* parent_asset = nullptr;
    const media::MeshAsset::SubMesh* sub_mesh = nullptr;
    for (const auto& inst : instances_) {
        if (inst.geom_id == rh.hit.geomID) {
            layer = inst.layer_ref;
            parent_asset = inst.parent_asset;
            sub_mesh = inst.sub_mesh;
            break;
        }
    }

    if (!layer) return {{1, 0, 1}, 1.0f}; // Magenta error

    // Baricentrics from hit
    float u = rh.hit.u;
    float v = rh.hit.v;
    float w = 1.0f - u - v;

    // Normals: Geometric fallback, but prefer Interpolated if mesh exists
    math::Vector3 normal = {rh.hit.Ng_x, rh.hit.Ng_y, rh.hit.Ng_z};
    normal = normal.normalized();
    
    if (sub_mesh && !sub_mesh->vertices.empty()) {
        unsigned int prim_id = rh.hit.primID;
        unsigned int i0 = sub_mesh->indices[prim_id * 3 + 0];
        unsigned int i1 = sub_mesh->indices[prim_id * 3 + 1];
        unsigned int i2 = sub_mesh->indices[prim_id * 3 + 2];
        
        // Interpolate vertex normals
        math::Vector3 n0 = sub_mesh->vertices[i0].normal;
        math::Vector3 n1 = sub_mesh->vertices[i1].normal;
        math::Vector3 n2 = sub_mesh->vertices[i2].normal;
        normal = (n0 * w + n1 * u + n2 * v).normalized();
    }

    // View vector
    math::Vector3 view_vec = -direction;
    float n_dot_v = std::clamp(math::Vector3::dot(normal, view_vec), 0.0f, 1.0f);
    math::Vector3 hit_pos = origin + direction * rh.ray.tfar;

    // UV Interpolation
    math::Vector2 uv{0, 0};
    if (sub_mesh) {
        unsigned int prim_id = rh.hit.primID;
        unsigned int i0 = sub_mesh->indices[prim_id * 3 + 0];
        unsigned int i1 = sub_mesh->indices[prim_id * 3 + 1];
        unsigned int i2 = sub_mesh->indices[prim_id * 3 + 2];
        uv = sub_mesh->vertices[i0].uv * w + sub_mesh->vertices[i1].uv * u + sub_mesh->vertices[i2].uv * v;
    } else {
        if (rh.hit.primID == 0) uv = math::Vector2{0,0} * (1-u-v) + math::Vector2{1,0} * u + math::Vector2{1,1} * v;
        else uv = math::Vector2{0,0} * (1-u-v) + math::Vector2{1,1} * u + math::Vector2{0,1} * v;
    }
    
    // Base color
    math::Vector3 albedo = {layer->fill_color.r / 255.0f, layer->fill_color.g / 255.0f, layer->fill_color.b / 255.0f};
    float metallic = layer->material.metallic;
    float roughness = std::max(layer->material.roughness, 0.05f);

    if (sub_mesh) {
        albedo = albedo * sub_mesh->material.base_color_factor;
        metallic = metallic * sub_mesh->material.metallic_factor;
        roughness = std::max(roughness * sub_mesh->material.roughness_factor, 0.05f);

        // Sample baseColorTexture
        if (sub_mesh->material.base_color_texture_idx >= 0 && parent_asset && sub_mesh->material.base_color_texture_idx < static_cast<int>(parent_asset->textures.size())) {
            const auto& tex = parent_asset->textures[sub_mesh->material.base_color_texture_idx];
            if (!tex.data.empty() && tex.width > 0 && tex.height > 0 && tex.channels > 0) {
                // glTF UVs can wrap or clamp; simply repeating for now
                float wrap_u = uv.x - std::floor(uv.x);
                float wrap_v = uv.y - std::floor(uv.y);
                int tx = std::clamp(static_cast<int>(wrap_u * tex.width), 0, tex.width - 1);
                int ty = std::clamp(static_cast<int>(wrap_v * tex.height), 0, tex.height - 1);
                int idx = (ty * tex.width + tx) * tex.channels;
                albedo = albedo * math::Vector3{ tex.data[idx+0]/255.0f, tex.data[idx+1]/255.0f, tex.data[idx+2]/255.0f };
            }
        }
        
        // Sample metallicRoughnessTexture
        if (sub_mesh->material.metallic_roughness_texture_idx >= 0 && parent_asset && sub_mesh->material.metallic_roughness_texture_idx < static_cast<int>(parent_asset->textures.size())) {
            const auto& tex = parent_asset->textures[sub_mesh->material.metallic_roughness_texture_idx];
            if (!tex.data.empty() && tex.width > 0 && tex.height > 0 && tex.channels >= 3) {
                float wrap_u = uv.x - std::floor(uv.x);
                float wrap_v = uv.y - std::floor(uv.y);
                int tx = std::clamp(static_cast<int>(wrap_u * tex.width), 0, tex.width - 1);
                int ty = std::clamp(static_cast<int>(wrap_v * tex.height), 0, tex.height - 1);
                int idx = (ty * tex.width + tx) * tex.channels;
                // glTF spec: Blue channel is metallic, Green channel is roughness
                roughness = std::max(roughness * (tex.data[idx+1]/255.0f), 0.05f);
                metallic = metallic * (tex.data[idx+2]/255.0f);
            }
        }
    } else {
        // Fallback to Layer Rendered Texture
        if (layer->texture_rgba && layer->width > 0 && layer->height > 0) {
            float wrap_u = uv.x - std::floor(uv.x);
            float wrap_v = uv.y - std::floor(uv.y);
            int tx = std::clamp(static_cast<int>(wrap_u * layer->width), 0, (int)layer->width - 1);
            int ty = std::clamp(static_cast<int>(wrap_v * layer->height), 0, (int)layer->height - 1);
            const uint8_t* tex_ptr = layer->texture_rgba + (ty * layer->width + tx) * 4;
            albedo = { tex_ptr[0] / 255.0f, tex_ptr[1] / 255.0f, tex_ptr[2] / 255.0f };
        }
    }

    math::Vector3 f0 = math::Vector3{0.04f, 0.04f, 0.04f} * (1.0f - metallic) + albedo * metallic;

    math::Vector3 total_lighting = {0.0f, 0.0f, 0.0f};
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    // Direct lighting
    for (const auto& light : state.lights) {
        math::Vector3 l_dir;
        float distance = INFINITY;
        float attenuation = 1.0f;

        if (light.type == "ambient") {
            total_lighting += albedo * light.color.to_vector3() * light.intensity * 0.1f;
            continue;
        } else if (light.type == "parallel") {
            l_dir = -light.direction;
        } else {
            math::Vector3 light_pos = light.position;
            if (light.shadow_radius > 0.0f) {
                math::Vector2 l_offset = sample_disk(light.shadow_radius, dist(rng), dist(rng));
                math::Vector3 l_safe_dir = light.direction.length_squared() > 1e-6f ? light.direction : math::Vector3{0, 0, -1};
                math::Vector3 l_right = (std::abs(l_safe_dir.y) > 0.99f) ? math::Vector3{1,0,0} : math::Vector3{0,1,0};
                math::Vector3 l_up = math::Vector3::cross(l_safe_dir, l_right).normalized();
                l_right = math::Vector3::cross(l_up, l_safe_dir).normalized();
                light_pos += l_right * l_offset.x + l_up * l_offset.y;
            }
            math::Vector3 delta = light_pos - hit_pos;
            distance = delta.length();
            l_dir = delta / distance;

            // Attenuation (simplified)
            if (distance > 0.0f) {
                if (light.falloff_type == "inverse_square") attenuation = 1.0f / (distance * distance);
                else if (light.falloff_type == "linear") attenuation = std::clamp(1.0f - (distance - light.attenuation_near) / (light.attenuation_far - light.attenuation_near), 0.0f, 1.0f);
                else {
                    float d = std::clamp((distance - light.attenuation_near) / (light.attenuation_far - light.attenuation_near), 0.0f, 1.0f);
                    attenuation = 1.0f - d * d * (3.0f - 2.0f * d);
                }
            }

            if (light.type == "spot") {
                float cos_theta = math::Vector3::dot(l_dir, -light.direction);
                float full_angle_rad = light.cone_angle * PI / 180.0f;
                float cos_outer = std::cos(full_angle_rad * 0.5f);
                float cos_inner = std::cos(std::max(0.0f, full_angle_rad * 0.5f - (light.cone_feather / 100.0f) * full_angle_rad * 0.5f));
                attenuation *= std::clamp((cos_theta - cos_outer) / (cos_inner - cos_outer), 0.0f, 1.0f);
            }
        }

        if (attenuation <= 0.0f) continue;

        // Shadow ray
        bool in_shadow = false;
        if (light.casts_shadows) {
            alignas(16) RTCRay shadow_ray{};
            shadow_ray.org_x = hit_pos.x + normal.x * 0.01f;
            shadow_ray.org_y = hit_pos.y + normal.y * 0.01f;
            shadow_ray.org_z = hit_pos.z + normal.z * 0.01f;
            shadow_ray.dir_x = l_dir.x; shadow_ray.dir_y = l_dir.y; shadow_ray.dir_z = l_dir.z;
            shadow_ray.tnear = 0.0f;
            shadow_ray.tfar = distance - 0.02f;
            shadow_ray.mask = static_cast<unsigned int>(-1);
            RTCOccludedArguments o_args;
            rtcInitOccludedArguments(&o_args);
            rtcOccluded1(scene_, reinterpret_cast<::RTCRay*>(&shadow_ray), &o_args);
            if (shadow_ray.tfar < 0.0f) in_shadow = true;
        }

        if (!in_shadow) {
            math::Vector3 h = (view_vec + l_dir).normalized();
            float n_dot_l = std::clamp(math::Vector3::dot(normal, l_dir), 0.0f, 1.0f);
            float n_dot_h = std::clamp(math::Vector3::dot(normal, h), 0.0f, 1.0f);
            float v_dot_h = std::clamp(math::Vector3::dot(view_vec, h), 0.0f, 1.0f);

            float d = ndf_ggx(n_dot_h, roughness);
            float g = geometry_smith(n_dot_v, n_dot_l, roughness);
            math::Vector3 ks = fresnel_schlick(v_dot_h, f0); 
            math::Vector3 kd = (math::Vector3{1,1,1} - ks) * (1.0f - metallic);

            math::Vector3 diffuse = kd * albedo * (1.0f / PI);
            math::Vector3 specular = ks * (d * g / (4.0f * n_dot_v * n_dot_l + 0.001f));

            math::Vector3 light_rgb = {light.color.r / 255.0f, light.color.g / 255.0f, light.color.b / 255.0f};
            total_lighting += (diffuse + specular) * light_rgb * (light.intensity * attenuation) * n_dot_l;
        }
    }

    // Indirect lighting (GI)
    if (depth < kMaxBounces) {
        // 1. Specular Reflection
        float reflectivity = metallic;
        if (reflectivity < 0.01f) {
            reflectivity = fresnel_schlick(n_dot_v, {0.04f, 0.04f, 0.04f}).x;
        }

        if (reflectivity > 0.01f) {
            math::Vector3 refl_dir;
            if (roughness < 0.05f) {
                refl_dir = (direction - normal * 2.0f * math::Vector3::dot(direction, normal)).normalized();
            } else {
                math::Vector3 h = importance_sample_ggx(dist(rng), dist(rng), normal, roughness);
                refl_dir = (direction - h * 2.0f * math::Vector3::dot(direction, h)).normalized();
            }
            
            // Ensure reflection is above surface
            if (math::Vector3::dot(refl_dir, normal) > 0.0f) {
                math::Vector3 ks = fresnel_schlick(std::clamp(math::Vector3::dot(view_vec, normal), 0.0f, 1.0f), f0);
                total_lighting += ks * trace_ray(hit_pos + normal * 0.001f, refl_dir, state, rng, depth + 1).color;
            }
        }

        // 2. Refraction / Transmission
        float transmission = 1.0f - layer->opacity;
        if (transmission > 0.01f) {
            float eta = 1.0f / layer->material.ior;
            math::Vector3 n = normal;
            if (math::Vector3::dot(direction, n) > 0) {
                n = -normal;
                eta = layer->material.ior;
            }

            float cos_i = -math::Vector3::dot(direction, n);
            float sin2_t = eta * eta * (1.0f - cos_i * cos_i);
            
            if (sin2_t <= 1.0f) {
                float cos_t = std::sqrt(1.0f - sin2_t);
                math::Vector3 refract_dir = (direction * eta + n * (eta * cos_i - cos_t)).normalized();
                auto refr_result = trace_ray(hit_pos - n * 0.001f, refract_dir, state, rng, depth + 1);
                total_lighting = total_lighting * (1.0f - transmission) + refr_result.color * transmission;
            }
        }

        // 3. Diffuse GI (Simple path tracing bounce)
        if (transmission < 0.9f && metallic < 0.9f) {
            float r1 = dist(rng);
            float r2 = dist(rng);
            float phi = 2.0f * PI * r1;
            float r = std::sqrt(r2);
            math::Vector3 local_dir = { r * std::cos(phi), r * std::sin(phi), std::sqrt(1.0f - r2) };
            
            math::Vector3 tangent = (std::abs(normal.x) > 0.1f) ? math::Vector3{0, 1, 0} : math::Vector3{1, 0, 0};
            math::Vector3 bitangent = math::Vector3::cross(normal, tangent).normalized();
            tangent = math::Vector3::cross(bitangent, normal).normalized();
            
            math::Vector3 diffuse_dir = tangent * local_dir.x + bitangent * local_dir.y + normal * local_dir.z;
            
            math::Vector3 ks = fresnel_schlick(n_dot_v, f0);
            math::Vector3 kd = (math::Vector3{1,1,1} - ks) * (1.0f - metallic);
            
            auto gi_result = trace_ray(hit_pos + normal * 0.001f, diffuse_dir, state, rng, depth + 1);
            total_lighting += kd * albedo * gi_result.color;
        }
    } else {
        // Fallback ambient / IBL (Split-Sum Approximation)
        if (environment_map_) {
            math::Vector3 ks = fresnel_schlick(n_dot_v, f0);
            math::Vector3 kd = (math::Vector3{1,1,1} - ks) * (1.0f - metallic);
            
            // Diffuse IBL (very blurry env map)
            math::Vector3 diffuse_env = sample_environment(normal, 1.0f).color;
            
            // Specular IBL (Split-Sum)
            math::Vector3 refl_dir = (direction - normal * 2.0f * math::Vector3::dot(direction, normal)).normalized();
            math::Vector3 specular_env = sample_environment(refl_dir, roughness).color;
            
            // LUT lookup
            float lut_u = std::clamp(n_dot_v, 0.0f, 1.0f);
            float lut_v = std::clamp(roughness, 0.0f, 1.0f);
            int lut_res = brdf_lut_ ? brdf_lut_->size : 0;
            float scale = 1.0f, bias = 0.0f;
            if (lut_res > 0) {
                int lx = std::clamp(static_cast<int>(lut_v * (lut_res - 1)), 0, lut_res - 1);
                int ly = std::clamp(static_cast<int>(lut_u * (lut_res - 1)), 0, lut_res - 1);
                scale = brdf_lut_->data[(ly * lut_res + lx) * 2 + 0];
                bias = brdf_lut_->data[(ly * lut_res + lx) * 2 + 1];
            }
            
            total_lighting += kd * albedo * diffuse_env + specular_env * (ks * scale + bias);
        } else {
            total_lighting += albedo * 0.03f;
        }
    }

    total_lighting += albedo * layer->material.emission;
    return {total_lighting, static_cast<float>(layer->opacity), rh.ray.tfar};
}

void RayTracer::render(const scene::EvaluatedCompositionState& state, float* out_rgba, float* out_depth, int width, int height) {
    if (!scene_ || instances_.empty()) return;
    const auto& eval_cam = state.camera;
    if (!eval_cam.available) return;

    math::Vector3 cam_pos = eval_cam.position;
    math::Vector3 forward = eval_cam.camera.transform.rotation.rotate_vector({0, 0, -1}).normalized();
    math::Vector3 right   = eval_cam.camera.transform.rotation.rotate_vector({1, 0, 0}).normalized();
    math::Vector3 up      = eval_cam.camera.transform.rotation.rotate_vector({0, 1, 0}).normalized();

    float tan_half_fov = std::tan(eval_cam.camera.fov_y_rad * 0.5f);
    float aspect = eval_cam.camera.aspect;

    #pragma omp parallel for schedule(dynamic)
    for (int y = 0; y < height; ++y) {
        std::mt19937 rng(static_cast<unsigned int>(state.frame_number + y * width));
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        for (int x = 0; x < width; ++x) {
            math::Vector3 accumulated_rgb{0,0,0};
            float accumulated_alpha = 0.0f;
            float accumulated_depth = 0.0f;
            int spp = std::max(1, samples_per_pixel_);

            for (int s = 0; s < spp; ++s) {
                float u = (x + (spp > 1 ? dist(rng) : 0.5f)) / width;
                float v = (y + (spp > 1 ? dist(rng) : 0.5f)) / height;

                // Motion Blur jitter (crude: we don't re-evaluate state, but we could jitter the time)
                // Real motion blur needs state evaluation at different times.
                // For now, let's just use the RNG to simulate it if someone wanted to.

                math::Vector3 ray_dir = (forward + right * (2*u-1)*aspect*tan_half_fov + up * (1-2*v)*tan_half_fov).normalized();
                math::Vector3 origin = cam_pos;
                
                if (eval_cam.dof_enabled && eval_cam.aperture > 0.0f) {
                    math::Vector3 focus_pt = cam_pos + ray_dir * eval_cam.focus_distance;
                    math::Vector2 lens_sample = sample_disk(eval_cam.aperture, dist(rng), dist(rng));
                    origin += right * lens_sample.x + up * lens_sample.y;
                    ray_dir = (focus_pt - origin).normalized();
                }

                auto result = trace_ray(origin, ray_dir, state, rng, 0);
                accumulated_rgb += result.color;
                accumulated_alpha += result.alpha;
                accumulated_depth += result.depth;
            }

            int idx = (y * width + x) * 4;
            out_rgba[idx+0] = accumulated_rgb.x / spp;
            out_rgba[idx+1] = accumulated_rgb.y / spp;
            out_rgba[idx+2] = accumulated_rgb.z / spp;
            out_rgba[idx+3] = std::clamp(accumulated_alpha / spp, 0.0f, 1.0f);
            
            if (out_depth) {
                out_depth[y * width + x] = accumulated_depth / spp;
            }
        }
    }

    // Apply OIDN denoising for low-SPP renders
    if (oidn_filter_ && samples_per_pixel_ <= 4) {
        oidn_filter_.setImage("color",  out_rgba, oidn::Format::Float3, static_cast<size_t>(width), static_cast<size_t>(height), 0, 4 * sizeof(float));
        oidn_filter_.setImage("output", out_rgba, oidn::Format::Float3, static_cast<size_t>(width), static_cast<size_t>(height), 0, 4 * sizeof(float));
        oidn_filter_.commit();
        oidn_filter_.execute();
    }
}

} // namespace renderer3d
} // namespace tachyon
