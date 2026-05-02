#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/scene3d_bridge.h"
#include "tachyon/renderer3d/core/evaluated_scene_3d.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/media/management/media_manager.h"
#include "tachyon/text/fonts/core/font_registry.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::filesystem::path fixture_path() {
    return std::filesystem::path(TACHYON_TESTS_SOURCE_DIR) / "fixtures" / "fonts" / "minimal_5x7.bdf";
}

std::filesystem::path image_fixture_path() {
    return std::filesystem::path(TACHYON_TESTS_SOURCE_DIR) / "fixtures" / "assets" / "logo.png";
}

std::filesystem::path repo_root_path() {
    std::filesystem::path root = std::filesystem::path(TACHYON_TESTS_SOURCE_DIR);
    for (int i = 0; i < 3; ++i) {
        const auto candidate = root / "src" / "renderer2d" / "evaluated_composition" / "rendering" / "pipeline" / "scene3d_bridge.cpp";
        if (std::filesystem::exists(candidate)) {
            return root;
        }
        if (!root.has_parent_path()) {
            break;
        }
        root = root.parent_path();
    }
    return std::filesystem::path(TACHYON_TESTS_SOURCE_DIR).parent_path();
}

std::filesystem::path bridge_source_path() {
    return repo_root_path() /
        "src" / "renderer2d" / "evaluated_composition" / "rendering" / "pipeline" / "scene3d_bridge.cpp";
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace

bool run_scene3d_bridge_tests() {
    g_failures = 0;

    // CameraContract test
    {
        tachyon::scene::EvaluatedCameraState camera;
        camera.available = true;
        camera.position = {1.0f, 2.0f, 3.0f};
        camera.point_of_interest = {0.0f, 0.0f, 0.0f};
        camera.angle_of_view = 45.0f;
        camera.focal_length = 50.0f;
        camera.focus_distance = 100.0f;
        camera.aperture = 2.8f;
        camera.layer_id = "camera1";

        tachyon::scene::EvaluatedCompositionState state;
        state.camera = camera;

        const auto result = tachyon::build_camera_3d(camera, state);
        check_true(result.position.x == 1.0f, "Camera position.x should be 1.0");
        check_true(result.target.x == 0.0f, "Camera target.x should be 0.0");
        check_true(result.is_active_camera, "Camera should be active");
        check_true(result.camera_id == "camera1", "Camera ID should match");
    }

    // MaterialContract test
    {
        tachyon::scene::EvaluatedCompositionState state;
        tachyon::Scene3DBridgeInput input;
        tachyon::scene::EvaluatedLayerState layer;
        layer.type = tachyon::scene::LayerType::Solid;
        layer.fill_color = {255, 0, 0, 255};
        layer.opacity = 0.5;
        layer.material.metallic = 0.8f;
        layer.material.roughness = 0.2f;
        layer.material.emission = 1.0f;
        layer.material.transmission = 0.5f;
        layer.material.ior = 1.5f;
        layer.world_matrix = tachyon::math::Matrix4x4::identity();
        layer.previous_world_matrix = tachyon::math::Matrix4x4::identity();

        std::vector<std::size_t> block_indices = {0};
        std::vector<bool> visible_3d_layers = {true};
        tachyon::renderer2d::RenderContext2D context;

        input.state = &state;
        input.block_indices = &block_indices;
        input.visible_3d_layers = &visible_3d_layers;
        input.context = &context;
        input.plan = nullptr;
        input.task = nullptr;

        state.layers.push_back(layer);

        const auto instances = tachyon::build_instances_3d(input);
        check_true(instances.size() == 1U, "Should have 1 instance");
        const auto& inst = instances.front();
        check_true(inst.material.base_color.r == 255, "Material red should be 255");
        check_true(inst.material.opacity == 0.5f, "Material opacity should be 0.5");
        check_true(inst.material.metallic == 0.8f, "Material metallic should be 0.8");
        check_true(inst.material.roughness == 0.2f, "Material roughness should be 0.2");
        check_true(inst.material.emission_strength == 1.0f, "Material emission should be 1.0");
        check_true(inst.material.transmission == 0.5f, "Material transmission should be 0.5");
        check_true(inst.material.ior == 1.5f, "Material IOR should be 1.5");
    }

    // UniversalSurfacePlane test
    {
        tachyon::scene::EvaluatedCompositionState state;
        tachyon::scene::EvaluatedLayerState layer;
        tachyon::renderer2d::RenderContext2D context;
        layer.type = tachyon::scene::LayerType::Solid;
        layer.is_3d = true;
        layer.visible = true;
        layer.enabled = true;
        layer.active = true;
        layer.width = 320;
        layer.height = 180;
        layer.fill_color = {20, 40, 60, 255};
        layer.world_matrix = tachyon::math::Matrix4x4::identity();
        layer.previous_world_matrix = tachyon::math::Matrix4x4::identity();

        state.layers.push_back(layer);

        tachyon::Scene3DBridgeInput input;
        input.state = &state;
        input.context = &context;

        const auto surface = tachyon::build_layer_surface(state.layers[0], input);
        check_true(surface.valid(), "Surface should be built for solid layers");
        if (surface.valid()) {
            check_true(surface.surface->width() == 320U, "Surface width should match layer width");
            check_true(surface.surface->height() == 180U, "Surface height should match layer height");
            check_true(!surface.cache_key.empty(), "Surface cache key should be set");
        }
    }

    // ImageSurface test
    {
        tachyon::media::MediaManager media_manager;
        tachyon::renderer2d::RenderContext2D context;
        context.media_manager = &media_manager;

        const auto image_path = image_fixture_path();
        check_true(std::filesystem::exists(image_path), "Test image should exist");

        if (std::filesystem::exists(image_path)) {
            tachyon::scene::EvaluatedLayerState layer;
            layer.type = tachyon::scene::LayerType::Image;
            layer.is_3d = true;
            layer.visible = true;
            layer.enabled = true;
            layer.active = true;
            layer.asset_path = image_path.string();
            layer.width = 256;
            layer.height = 256;
            layer.world_matrix = tachyon::math::Matrix4x4::identity();
            layer.previous_world_matrix = tachyon::math::Matrix4x4::identity();

            tachyon::scene::EvaluatedCompositionState state;
            state.layers.push_back(layer);

            tachyon::Scene3DBridgeInput input;
            input.state = &state;
            input.context = &context;

            const auto surface = tachyon::build_layer_surface(state.layers[0], input);
            check_true(surface.valid(), "Surface should be built for image layers");
            if (surface.valid()) {
                check_true(surface.surface->width() > 0U, "Image surface width should be positive");
                check_true(surface.surface->height() > 0U, "Image surface height should be positive");
            }
        }
    }

    // TextSurfacePlane test
    {
        tachyon::text::FontRegistry font_registry;
        std::filesystem::path font_path = fixture_path();
        check_true(std::filesystem::exists(font_path), "Test font should exist");
        
        if (std::filesystem::exists(font_path)) {
            check_true(font_registry.load_bdf("default", font_path), "Should load test font");

            tachyon::renderer2d::RenderContext2D context;
            context.font_registry = &font_registry;

            tachyon::scene::EvaluatedLayerState layer;
            layer.type = tachyon::scene::LayerType::Text;
            layer.is_3d = true;
            layer.visible = true;
            layer.enabled = true;
            layer.active = true;
            layer.text_content = "Hello";
            layer.font_id = "default";
            layer.font_size = 14.0f;
            layer.world_matrix = tachyon::math::Matrix4x4::identity();
            layer.previous_world_matrix = tachyon::math::Matrix4x4::identity();
            layer.width = 200;
            layer.height = 50;
            layer.fill_color = {255, 255, 255, 255};
            layer.opacity = 1.0f;

            tachyon::scene::EvaluatedCompositionState state;
            state.layers.push_back(layer);

            tachyon::Scene3DBridgeInput input;
            input.state = &state;
            input.context = &context;
            input.plan = nullptr;
            input.task = nullptr;

            std::vector<std::size_t> block_indices = {0};
            std::vector<bool> visible_3d_layers = {true};
            input.block_indices = &block_indices;
            input.visible_3d_layers = &visible_3d_layers;

            const auto instances = tachyon::build_instances_3d(input);

            check_true(instances.size() == 1U, "Should have 1 instance for text");
            if (!instances.empty()) {
                const auto& inst = instances.front();
                check_true(inst.mesh_asset != nullptr, "Text layer should build a shared 3D plane mesh");
                check_true(!inst.mesh_asset_id.empty(), "Mesh asset ID should be set");
            }
        }
    }

    // BridgeArchitecture test
    {
        const auto source_path = bridge_source_path();
        check_true(std::filesystem::exists(source_path), "Bridge source file should exist");

        if (std::filesystem::exists(source_path)) {
            const auto source = read_file(source_path);
            check_true(source.find("LayerType::Image") == std::string::npos, "Bridge should not branch on image type");
            check_true(source.find("LayerType::Video") == std::string::npos, "Bridge should not branch on video type");
            check_true(source.find("LayerType::Text") == std::string::npos, "Bridge should not branch on text type");
            check_true(source.find("LayerType::Shape") == std::string::npos, "Bridge should not branch on shape type");
            check_true(source.find("resolve_media_source") == std::string::npos, "Bridge should not resolve media sources");
        }
    }

    return g_failures == 0;
}
