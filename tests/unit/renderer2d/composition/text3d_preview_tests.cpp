#include "tachyon/renderer2d/evaluated_composition/rendering/text_mesh_builder.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/text/fonts/font_registry.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <iostream>
#include <optional>
#include <vector>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::filesystem::path find_preview_font() {
    const std::filesystem::path candidates[] = {
        "assets/fonts/SFPRODISPLAYBOLD.OTF",
        "fonts/SFPRODISPLAYBOLD.OTF",
        "tests/fixtures/fonts/SFPRODISPLAYBOLD.OTF",
        "build/_deps/harfbuzz-src/test/subset/data/fonts/Roboto-Regular.ttf",
        "_deps/harfbuzz-src/test/subset/data/fonts/Roboto-Regular.ttf",
        "../build/_deps/harfbuzz-src/test/subset/data/fonts/Roboto-Regular.ttf"
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}

struct ProjectedVertex {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
};

bool project_vertex(
    const tachyon::math::Vector3& position,
    const tachyon::math::Matrix4x4& view,
    const tachyon::math::Matrix4x4& projection,
    int width,
    int height,
    ProjectedVertex& out_vertex) {

    const tachyon::math::Matrix4x4 mvp = projection * view;
    const tachyon::math::Vector3 clip = mvp.transform_point(position);

    if (!std::isfinite(clip.x) || !std::isfinite(clip.y) || !std::isfinite(clip.z)) {
        return false;
    }

    out_vertex.x = (clip.x * 0.5f + 0.5f) * static_cast<float>(width - 1);
    out_vertex.y = (1.0f - (clip.y * 0.5f + 0.5f)) * static_cast<float>(height - 1);
    out_vertex.z = clip.z;
    return clip.z >= -1.0f && clip.z <= 1.0f;
}

float edge_function(const ProjectedVertex& a, const ProjectedVertex& b, float x, float y) {
    return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
}

void draw_filled_triangle(
    tachyon::renderer2d::SurfaceRGBA& surface,
    std::vector<float>& depth_buffer,
    int width,
    int height,
    const ProjectedVertex& v0,
    const ProjectedVertex& v1,
    const ProjectedVertex& v2,
    const tachyon::renderer2d::Color& color) {

    const float min_x_f = std::floor(std::min({v0.x, v1.x, v2.x}));
    const float min_y_f = std::floor(std::min({v0.y, v1.y, v2.y}));
    const float max_x_f = std::ceil(std::max({v0.x, v1.x, v2.x}));
    const float max_y_f = std::ceil(std::max({v0.y, v1.y, v2.y}));

    const int min_x = std::max(0, static_cast<int>(min_x_f));
    const int min_y = std::max(0, static_cast<int>(min_y_f));
    const int max_x = std::min(width - 1, static_cast<int>(max_x_f));
    const int max_y = std::min(height - 1, static_cast<int>(max_y_f));

    const float area = edge_function(v0, v1, v2.x, v2.y);
    if (std::abs(area) < 1.0e-6f) {
        return;
    }

    const bool is_ccw = area > 0.0f;

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            const float px = static_cast<float>(x) + 0.5f;
            const float py = static_cast<float>(y) + 0.5f;

            const float w0 = edge_function(v1, v2, px, py);
            const float w1 = edge_function(v2, v0, px, py);
            const float w2 = edge_function(v0, v1, px, py);

            const bool inside = is_ccw ? (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) : (w0 <= 0.0f && w1 <= 0.0f && w2 <= 0.0f);
            if (!inside) {
                continue;
            }

            const float inv_area = 1.0f / area;
            const float b0 = w0 * inv_area;
            const float b1 = w1 * inv_area;
            const float b2 = w2 * inv_area;
            const float z = b0 * v0.z + b1 * v1.z + b2 * v2.z;

            const std::size_t depth_index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
            if (depth_index >= depth_buffer.size() || z >= depth_buffer[depth_index]) {
                continue;
            }

            depth_buffer[depth_index] = z;
            surface.set_pixel(x, y, color);
        }
    }
}

bool save_mesh_preview_png(
    const tachyon::media::MeshAsset& mesh,
    const std::filesystem::path& out_png) {

    using namespace tachyon;

    constexpr int kWidth = 1280;
    constexpr int kHeight = 720;

    renderer2d::SurfaceRGBA surface(static_cast<std::uint32_t>(kWidth), static_cast<std::uint32_t>(kHeight));
    surface.clear(renderer2d::Color{0.05f, 0.06f, 0.08f, 1.0f});

    std::vector<float> depth_buffer(static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight), std::numeric_limits<float>::infinity());

    bool have_bounds = false;
    math::Vector3 bounds_min{0.0f, 0.0f, 0.0f};
    math::Vector3 bounds_max{0.0f, 0.0f, 0.0f};

    for (const auto& submesh : mesh.sub_meshes) {
        for (const auto& vertex : submesh.vertices) {
            const math::Vector3 transformed = submesh.transform.transform_point(vertex.position);
            if (!have_bounds) {
                bounds_min = bounds_max = transformed;
                have_bounds = true;
            } else {
                bounds_min.x = std::min(bounds_min.x, transformed.x);
                bounds_min.y = std::min(bounds_min.y, transformed.y);
                bounds_min.z = std::min(bounds_min.z, transformed.z);
                bounds_max.x = std::max(bounds_max.x, transformed.x);
                bounds_max.y = std::max(bounds_max.y, transformed.y);
                bounds_max.z = std::max(bounds_max.z, transformed.z);
            }
        }
    }

    if (!have_bounds) {
        return false;
    }

    const math::Vector3 center = (bounds_min + bounds_max) * 0.5f;
    const math::Vector3 size = bounds_max - bounds_min;
    const float extent = std::max({size.x, size.y, size.z, 1.0f});
    const float unit_scale = 2.8f / extent;

    const math::Quaternion rotation = math::Quaternion::from_axis_angle({0.0f, 1.0f, 0.0f}, 22.0f * 3.1415926535f / 180.0f);
    const math::Matrix4x4 view = math::Matrix4x4::look_at({0.0f, 0.15f, 4.1f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    const math::Matrix4x4 projection = math::Matrix4x4::perspective(45.0f * 3.1415926535f / 180.0f, static_cast<float>(kWidth) / static_cast<float>(kHeight), 0.1f, 100.0f);

    const math::Vector3 light_dir = math::Vector3{-0.55f, 0.35f, 1.0f}.normalized();

    for (const auto& submesh : mesh.sub_meshes) {
        for (std::size_t i = 0; i + 2 < submesh.indices.size(); i += 3) {
            const unsigned int i0 = submesh.indices[i + 0];
            const unsigned int i1 = submesh.indices[i + 1];
            const unsigned int i2 = submesh.indices[i + 2];
            if (i0 >= submesh.vertices.size() || i1 >= submesh.vertices.size() || i2 >= submesh.vertices.size()) {
                continue;
            }

            const auto world0 = submesh.transform.transform_point(submesh.vertices[i0].position) - center;
            const auto world1 = submesh.transform.transform_point(submesh.vertices[i1].position) - center;
            const auto world2 = submesh.transform.transform_point(submesh.vertices[i2].position) - center;

            const auto scaled0 = world0 * unit_scale;
            const auto scaled1 = world1 * unit_scale;
            const auto scaled2 = world2 * unit_scale;

            const auto rotated0 = rotation.rotate_vector(scaled0);
            const auto rotated1 = rotation.rotate_vector(scaled1);
            const auto rotated2 = rotation.rotate_vector(scaled2);

            const math::Vector3 fixed0{rotated0.x, -rotated0.y, rotated0.z};
            const math::Vector3 fixed1{rotated1.x, -rotated1.y, rotated1.z};
            const math::Vector3 fixed2{rotated2.x, -rotated2.y, rotated2.z};

            ProjectedVertex p0;
            ProjectedVertex p1;
            ProjectedVertex p2;
            if (!project_vertex(fixed0, view, projection, kWidth, kHeight, p0)) {
                continue;
            }
            if (!project_vertex(fixed1, view, projection, kWidth, kHeight, p1)) {
                continue;
            }
            if (!project_vertex(fixed2, view, projection, kWidth, kHeight, p2)) {
                continue;
            }

            const math::Vector3 face_normal = math::Vector3::cross(fixed1 - fixed0, fixed2 - fixed0).normalized();
            const float ndotl = std::max(0.0f, math::Vector3::dot(face_normal, light_dir));
            const float shade = 0.22f + 0.78f * std::max(ndotl, 0.16f);

            const renderer2d::Color face_color{
                0.94f * shade,
                0.84f * shade,
                0.62f * shade,
                1.0f
            };

            draw_filled_triangle(surface, depth_buffer, kWidth, kHeight, p0, p1, p2, face_color);
        }
    }

    std::filesystem::create_directories(out_png.parent_path());
    return surface.save_png(out_png);
}

} // namespace

bool run_text3d_preview_tests() {
    using namespace tachyon;

    g_failures = 0;

    const std::filesystem::path font_path = find_preview_font();
    check_true(!font_path.empty(), "Preview font fixture should exist");
    if (font_path.empty()) {
        return false;
    }

    text::FontRegistry font_registry;
    check_true(font_registry.load_ttf("SFProDisplayBold", font_path, 96), "Preview font should load");
    check_true(font_registry.set_default("SFProDisplayBold"), "Preview font should become default font");

    scene::EvaluatedCompositionState state;
    state.composition_id = "text3d_preview";
    state.width = 1280;
    state.height = 720;
    state.frame_rate.numerator = 30;
    state.frame_rate.denominator = 1;
    state.frame_number = 0;

    scene::EvaluatedLayerState text_layer;
    text_layer.id = "title";
    text_layer.name = "Title";
    text_layer.type = scene::LayerType::Text;
    text_layer.enabled = true;
    text_layer.active = true;
    text_layer.visible = true;
    text_layer.is_3d = true;
    text_layer.width = 1280;
    text_layer.height = 720;
    text_layer.text_content = "TACHYON";
    text_layer.font_id = "SFProDisplayBold";
    text_layer.font_size = 96.0f;
    text_layer.fill_color = ColorSpec{235, 240, 255, 255};
    text_layer.text_alignment = 0;
    text_layer.extrusion_depth = 0.18f;
    text_layer.bevel_size = 0.02f;
    text_layer.world_matrix = math::compose_trs(
        {0.0f, 0.0f, 0.0f},
        math::Quaternion::from_axis_angle({0.0f, 1.0f, 0.0f}, 30.0f * 3.14159265f / 180.0f),
        {1.0f, 1.0f, 1.0f});
    text_layer.previous_world_matrix = text_layer.world_matrix;
    text_layer.layer_index = 0;
    state.layers.push_back(text_layer);

    const auto mesh = renderer2d::build_text_extrusion_mesh(state.layers.front(), state, font_registry);
    check_true(static_cast<bool>(mesh.mesh), "text extrusion mesh should be generated");
    if (mesh.mesh) {
        check_true(!mesh.mesh->sub_meshes.empty(), "text extrusion mesh should contain submeshes");
    }

    if (mesh.mesh) {
        const std::filesystem::path out_png = "tests/output/text3d_preview/text3d_preview.png";
        check_true(save_mesh_preview_png(*mesh.mesh, out_png), "3D text preview PNG should save");
        check_true(std::filesystem::exists(out_png), "3D text preview PNG should exist on disk");
    }

    return g_failures == 0;
}
