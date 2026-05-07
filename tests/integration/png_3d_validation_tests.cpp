#include "tachyon/scene/builder.h"
#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/scene3d_bridge.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/media/management/image_manager.h"
#include "tachyon/text/fonts/core/font_registry.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

const std::filesystem::path& tests_root() {
    static const std::filesystem::path root = TACHYON_TESTS_SOURCE_DIR;
    return root;
}

std::filesystem::path validation_root() {
    const std::filesystem::path root = tests_root() / ".." / "output" / "png_3d_validation";
    std::filesystem::create_directories(root);
    return root;
}

float smoothstep(float edge0, float edge1, float x) {
    if (edge0 == edge1) {
        return x < edge0 ? 0.0f : 1.0f;
    }
    const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

const tachyon::CompositionSpec* find_composition(
    const tachyon::SceneSpec& scene,
    const std::string& composition_id) {

    for (const auto& composition : scene.compositions) {
        if (composition.id == composition_id) {
            return &composition;
        }
    }

    return nullptr;
}

void put_soft_disc(
    tachyon::renderer2d::SurfaceRGBA& surface,
    float cx,
    float cy,
    float radius,
    const tachyon::renderer2d::Color& color,
    float softness) {

    for (std::uint32_t y = 0; y < surface.height(); ++y) {
        for (std::uint32_t x = 0; x < surface.width(); ++x) {
            const float dx = static_cast<float>(x) + 0.5f - cx;
            const float dy = static_cast<float>(y) + 0.5f - cy;
            const float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius + softness) {
                continue;
            }
            const float falloff = 1.0f - smoothstep(radius, radius + softness, dist);
            tachyon::renderer2d::Color px = color;
            px.a *= falloff;
            px.r *= px.a;
            px.g *= px.a;
            px.b *= px.a;
            surface.blend_pixel(x, y, px);
        }
    }
}

void put_rounded_logo(
    tachyon::renderer2d::SurfaceRGBA& surface,
    const tachyon::renderer2d::Color& fill,
    const tachyon::renderer2d::Color& shadow,
    float border_radius) {

    const float cx = static_cast<float>(surface.width()) * 0.5f;
    const float cy = static_cast<float>(surface.height()) * 0.5f;
    const float shadow_cx = cx + 16.0f;
    const float shadow_cy = cy + 18.0f;
    const float half_w = static_cast<float>(surface.width()) * 0.29f;
    const float half_h = static_cast<float>(surface.height()) * 0.29f;

    for (std::uint32_t y = 0; y < surface.height(); ++y) {
        for (std::uint32_t x = 0; x < surface.width(); ++x) {
            const float fx = static_cast<float>(x) + 0.5f;
            const float fy = static_cast<float>(y) + 0.5f;

            const float shadow_dx = std::abs(fx - shadow_cx) - (half_w + 8.0f);
            const float shadow_dy = std::abs(fy - shadow_cy) - (half_h + 8.0f);
            const float shadow_outer = std::max(shadow_dx, shadow_dy);
            if (shadow_outer < 22.0f) {
                const float alpha = 0.22f * (1.0f - smoothstep(0.0f, 22.0f, shadow_outer));
                surface.blend_pixel(x, y, tachyon::renderer2d::Color{shadow.r, shadow.g, shadow.b, alpha});
            }
        }
    }

    for (std::uint32_t y = 0; y < surface.height(); ++y) {
        for (std::uint32_t x = 0; x < surface.width(); ++x) {
            const float fx = static_cast<float>(x) + 0.5f;
            const float fy = static_cast<float>(y) + 0.5f;
            const float dx = std::abs(fx - cx) - half_w;
            const float dy = std::abs(fy - cy) - half_h;
            const float outer = std::max(dx, dy);
            if (outer > border_radius) {
                continue;
            }
            const float alpha = 1.0f - smoothstep(0.0f, border_radius, outer);
            tachyon::renderer2d::Color px = fill;
            px.a *= alpha;
            px.r *= px.a;
            px.g *= px.a;
            px.b *= px.a;
            surface.blend_pixel(x, y, px);
        }
    }
}

void write_test_assets(const std::filesystem::path& asset_dir) {
    std::filesystem::create_directories(asset_dir);

    {
        tachyon::renderer2d::SurfaceRGBA surface(128, 128);
        surface.clear(tachyon::renderer2d::Color{0.83f, 0.12f, 0.12f, 1.0f});
        surface.save_png(asset_dir / "opaque_square.png");
    }

    {
        tachyon::renderer2d::SurfaceRGBA surface(256, 256);
        surface.clear(tachyon::renderer2d::Color::transparent());
        put_rounded_logo(
            surface,
            tachyon::renderer2d::Color{0.16f, 0.48f, 0.92f, 1.0f},
            tachyon::renderer2d::Color{0.0f, 0.0f, 0.0f, 1.0f},
            18.0f);
        surface.save_png(asset_dir / "logo_alpha.png");
    }

    {
        tachyon::renderer2d::SurfaceRGBA surface(256, 256);
        surface.clear(tachyon::renderer2d::Color::transparent());
        put_soft_disc(
            surface,
            128.0f,
            118.0f,
            62.0f,
            tachyon::renderer2d::Color{0.0f, 0.0f, 0.0f, 0.42f},
            34.0f);
        put_soft_disc(
            surface,
            128.0f,
            102.0f,
            52.0f,
            tachyon::renderer2d::Color{0.92f, 0.92f, 0.98f, 0.88f},
            18.0f);
        surface.save_png(asset_dir / "semi_transparent_shadow.png");
    }

    {
        tachyon::renderer2d::SurfaceRGBA surface(150, 90);
        surface.clear(tachyon::renderer2d::Color{0.07f, 0.08f, 0.10f, 1.0f});
        for (std::uint32_t y = 0; y < surface.height(); ++y) {
            for (std::uint32_t x = 0; x < surface.width(); ++x) {
                const bool stripe = ((x / 10U) + (y / 9U)) % 2U == 0U;
                const tachyon::renderer2d::Color color = stripe
                    ? tachyon::renderer2d::Color{0.84f, 0.74f, 0.16f, 1.0f}
                    : tachyon::renderer2d::Color{0.18f, 0.72f, 0.76f, 1.0f};
                surface.set_pixel(x, y, color);
            }
        }
        surface.save_png(asset_dir / "non_power_of_two.png");
    }

    {
        tachyon::renderer2d::SurfaceRGBA surface(128, 128);
        surface.clear(tachyon::renderer2d::Color{0.92f, 0.12f, 0.10f, 1.0f});
        surface.save_png(asset_dir / "parallax_red.png");
    }

    {
        tachyon::renderer2d::SurfaceRGBA surface(128, 128);
        surface.clear(tachyon::renderer2d::Color{0.12f, 0.84f, 0.18f, 1.0f});
        surface.save_png(asset_dir / "parallax_green.png");
    }

    {
        tachyon::renderer2d::SurfaceRGBA surface(128, 128);
        surface.clear(tachyon::renderer2d::Color{0.16f, 0.28f, 0.94f, 1.0f});
        surface.save_png(asset_dir / "parallax_blue.png");
    }
}

std::filesystem::path ensure_test_assets() {
    const std::filesystem::path asset_dir = validation_root() / "assets";
    write_test_assets(asset_dir);
    return asset_dir;
}

struct ImageDiff {
    double mean_error{0.0};
    double max_error{0.0};
};

ImageDiff compare_surfaces(const tachyon::renderer2d::SurfaceRGBA& a, const tachyon::renderer2d::SurfaceRGBA& b) {
    ImageDiff diff;
    if (a.width() != b.width() || a.height() != b.height()) {
        diff.max_error = std::numeric_limits<double>::infinity();
        diff.mean_error = std::numeric_limits<double>::infinity();
        return diff;
    }

    const auto& pa = a.pixels();
    const auto& pb = b.pixels();
    const std::size_t pixel_count = std::min(pa.size(), pb.size());
    if (pixel_count == 0) {
        return diff;
    }

    double total = 0.0;
    for (std::size_t i = 0; i < pixel_count; ++i) {
        const double error = std::abs(static_cast<double>(pa[i]) - static_cast<double>(pb[i])) * 255.0;
        total += error;
        diff.max_error = std::max(diff.max_error, error);
    }
    diff.mean_error = total / static_cast<double>(pixel_count);
    return diff;
}

std::filesystem::path font_path() {
    const std::filesystem::path candidate = tests_root() / ".." / "assets" / "fonts" / "SegoeUI.ttf";
    if (std::filesystem::exists(candidate)) {
        return candidate;
    }
    return tests_root() / ".." / "assets" / "fonts" / "SFPRODISPLAYBOLD.OTF";
}

tachyon::SceneSpec make_scene_with_image(const std::filesystem::path& image_path, float camera_x = 0.0f) {
    using namespace tachyon::scene;

    return Composition("png_3d_smoke")
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .clear({20, 22, 28, 255})
        .camera3d_layer("cam", [camera_x](LayerBuilder& l) {
            l.position3d(camera_x, 0.0, -560.0)
             .camera_poi(0.0, 0.0, 0.0)
             .camera_zoom(40.0);
        })
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light_type("ambient")
             .light_color({255, 255, 255, 255})
             .light_intensity(0.65);
        })
        .light_layer("key", [](LayerBuilder& l) {
            l.light_type("point")
             .position3d(-220.0, 180.0, -250.0)
             .light_color({255, 248, 234, 255})
             .light_intensity(2.8);
        })
        .layer("logo", [&image_path](LayerBuilder& l) {
            l.image(image_path.string())
             .size(260, 260)
             .position3d(0.0, 0.0, 240.0)
             .rotation3d(0.0, 25.0, 0.0)
             .scale3d(1.0, 1.0, 1.0)
             .emission_strength(3.5);
        })
        .build_scene();
}

tachyon::SceneSpec make_reusable_3d_scene(const std::filesystem::path& image_path) {
    using namespace tachyon::scene;

    return Composition("png_3d_reuse")
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .clear({18, 20, 24, 255})
        .camera3d_layer("cam", [](LayerBuilder& l) {
            l.position3d(0.0, 0.0, -580.0)
             .camera_poi(0.0, 0.0, 0.0)
             .camera_zoom(42.0);
        })
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light_type("ambient")
             .light_color({255, 255, 255, 255})
             .light_intensity(0.75);
        })
        .layer("image", [&image_path](LayerBuilder& l) {
            l.image(image_path.string())
             .size(220, 220)
             .position3d(-280.0, 0.0, 260.0)
             .rotation3d(0.0, 18.0, 0.0)
             .emission_strength(2.0);
        })
        .layer("text", [](LayerBuilder& l) {
            l.text("TACHYON")
             .font("default")
             .font_size(54.0)
             .fill_color({245, 245, 248, 255})
             .size(360, 120)
             .position3d(0.0, 20.0, 340.0)
             .rotation3d(0.0, -12.0, 0.0)
             .emission_strength(1.5);
        })
        .layer("shape", [](LayerBuilder& l) {
            l.type("shape")
             .kind(tachyon::LayerType::Shape)
             .fill_color({235, 142, 64, 255})
             .size(230, 230)
             .position3d(300.0, 0.0, 460.0)
             .rotation3d(0.0, -20.0, 0.0)
             .emission_strength(1.0);
        })
        .build_scene();
}

tachyon::SceneSpec make_parallax_layer_scene(const std::filesystem::path& image_path, float z, float camera_x, const std::string& scene_id) {
    using namespace tachyon::scene;

    return Composition(scene_id)
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .clear({14, 16, 20, 255})
        .camera3d_layer("cam", [camera_x](LayerBuilder& l) {
            l.position3d(camera_x, 0.0, -520.0)
             .camera_poi(camera_x, 0.0, 0.0)
             .camera_zoom(40.0);
        })
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light_type("ambient")
             .light_color({255, 255, 255, 255})
             .light_intensity(0.8);
        })
        .layer("card", [&image_path, z](LayerBuilder& l) {
            l.image(image_path.string())
             .size(160, 160)
             .position3d(0.0, 0.0, z)
             .emission_strength(4.0);
        })
        .build_scene();
}

tachyon::RasterizedFrame2D render_scene(
    const tachyon::SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number,
    tachyon::renderer2d::RenderContext2D& context,
    const std::function<void(tachyon::scene::EvaluatedCompositionState&)>& adjust_state = {}) {

    const std::filesystem::path output_path = validation_root() / (composition_id + "_" + std::to_string(frame_number) + ".png");
    const auto* composition = find_composition(scene, composition_id);
    if (composition == nullptr) {
        std::cerr << "FAIL: missing composition " << composition_id << '\n';
        return {};
    }

    tachyon::RenderPlan plan;
    plan.job_id = "png_3d_validation_" + composition_id + "_" + std::to_string(frame_number);
    plan.composition_target = composition_id;
    plan.scene_spec = &scene;
    plan.frame_range = {frame_number, frame_number};
    plan.output.destination.path = output_path.string();
    plan.output.destination.overwrite = true;
    plan.output.profile.name = "png-sequence";
    plan.output.profile.container = "png";
    plan.output.profile.format = tachyon::OutputFormat::ImageSequence;
    plan.output.profile.video.codec = "png";
    plan.output.profile.video.pixel_format = "rgba8";
    plan.composition.id = composition->id;
    plan.composition.name = composition->name;
    plan.composition.width = composition->width;
    plan.composition.height = composition->height;
    plan.composition.duration = composition->duration;
    plan.composition.frame_rate = composition->frame_rate;
    plan.composition.background = composition->background;
    plan.quality_policy.resolution_scale = 1.0f;

    auto state = tachyon::scene::evaluate_scene_composition_state(
        scene,
        composition_id,
        frame_number,
        nullptr,
        {},
        context.media_manager);

    if (!state.has_value()) {
        std::cerr << "FAIL: scene evaluation failed for " << composition_id << '\n';
        return {};
    }

    if (adjust_state) {
        adjust_state(*state);
    }

    tachyon::FrameRenderTask task;
    task.frame_number = frame_number;
    task.cache_key = {"png_3d_validation:" + composition_id + ":" + std::to_string(frame_number)};
    task.cacheable = true;
    const double fps = plan.composition.frame_rate.value() > 0.0
        ? plan.composition.frame_rate.value()
        : 30.0;
    task.time_seconds = static_cast<double>(frame_number) / fps;

    renderer2d::EffectRegistry effect_reg;
    RenderIntent intent_placeholder; // Since we are using an already evaluated state, intent is mostly used for resources if needed.
    return tachyon::render_evaluated_composition_2d(*state, intent_placeholder, plan, task, context, effect_reg);
}

bool check_png_decode(const std::filesystem::path& path, tachyon::media::ImageManager& image_manager) {
    tachyon::DiagnosticBag diagnostics;
    const auto* image = image_manager.get_image(path, tachyon::media::AlphaMode::Straight, &diagnostics);
    check_true(image != nullptr, "PNG should decode: " + path.string());
    check_true(diagnostics.ok(), "PNG decode should not emit errors: " + path.string());
    return image != nullptr && diagnostics.ok();
}

} // namespace

bool run_png_3d_validation_tests() {
    using namespace tachyon;

    g_failures = 0;

    const std::filesystem::path asset_dir = ensure_test_assets();
    const std::filesystem::path opaque_square = asset_dir / "opaque_square.png";
    const std::filesystem::path logo_alpha = asset_dir / "logo_alpha.png";
    const std::filesystem::path semi_transparent_shadow = asset_dir / "semi_transparent_shadow.png";
    const std::filesystem::path non_power_of_two = asset_dir / "non_power_of_two.png";

    {
        media::ImageManager image_manager;
        check_true(check_png_decode(opaque_square, image_manager), "opaque_square should decode");
        check_true(check_png_decode(logo_alpha, image_manager), "logo_alpha should decode");
        check_true(check_png_decode(semi_transparent_shadow, image_manager), "semi_transparent_shadow should decode");
        check_true(check_png_decode(non_power_of_two, image_manager), "non_power_of_two should decode");

        const auto* logo = image_manager.get_image(logo_alpha, media::AlphaMode::Straight, nullptr);
        if (logo != nullptr) {
            check_true(logo->width() == 256U, "logo_alpha width should be 256");
            check_true(logo->height() == 256U, "logo_alpha height should be 256");
            check_true(logo->get_pixel(10, 10).a < 0.1f, "logo_alpha corners should stay transparent after decode");
            check_true(logo->get_pixel(128, 128).b > 0.4f, "logo_alpha center should stay visibly blue");
        }
    }

    {
        RenderContext context;
        const auto rendered = render_scene(
            make_scene_with_image(logo_alpha),
            "png_3d_smoke",
            0,
            context.renderer2d,
            [&logo_alpha](tachyon::scene::EvaluatedCompositionState& state) {
                for (auto& layer : state.layers) {
                    if (layer.type == tachyon::LayerType::Image) {
                        layer.asset_path = logo_alpha.string();
                    }
                }
            });
        check_true(rendered.surface != nullptr, "3D image scene should render a surface");
        if (rendered.surface != nullptr) {
            const std::filesystem::path output_path = validation_root() / "png_3d_smoke_0.png";
            rendered.surface->save_png(output_path);
            check_true(rendered.surface->width() == 1280U, "3D render width should match scene");
            check_true(rendered.surface->height() == 720U, "3D render height should match scene");
            check_true(rendered.note.find("fallback") == std::string::npos, "3D image scene should use the ray-traced path");

            const auto center = rendered.surface->get_pixel(640, 360);
            const auto corner = rendered.surface->get_pixel(20, 20);
            check_true(center.b > center.r && center.b > center.g, "Rendered logo should remain blue-dominant");
            check_true(corner.a > 0.95f, "Background should stay opaque");
        }
    }

    {
        RenderContext first_context;
        RenderContext second_context;
        const auto first = render_scene(
            make_scene_with_image(logo_alpha),
            "png_3d_smoke",
            0,
            first_context.renderer2d,
            [&logo_alpha](tachyon::scene::EvaluatedCompositionState& state) {
                for (auto& layer : state.layers) {
                    if (layer.type == tachyon::LayerType::Image) {
                        layer.asset_path = logo_alpha.string();
                    }
                }
            });
        const auto second = render_scene(
            make_scene_with_image(logo_alpha),
            "png_3d_smoke",
            0,
            second_context.renderer2d,
            [&logo_alpha](tachyon::scene::EvaluatedCompositionState& state) {
                for (auto& layer : state.layers) {
                    if (layer.type == tachyon::LayerType::Image) {
                        layer.asset_path = logo_alpha.string();
                    }
                }
            });
        check_true(first.surface != nullptr && second.surface != nullptr, "Determinism renders should produce surfaces");
        if (first.surface != nullptr && second.surface != nullptr) {
            const ImageDiff diff = compare_surfaces(*first.surface, *second.surface);
            check_true(diff.mean_error <= 0.5, "Same scene and frame should render deterministically");
            check_true(diff.max_error <= 2.0, "Same scene and frame should not diverge beyond tolerance");
        }
    }

    {
        RenderContext context;
        text::FontRegistry font_registry;
        const std::filesystem::path font_file = font_path();
        check_true(std::filesystem::exists(font_file), "Test font should exist for 3D text reuse test");
        if (std::filesystem::exists(font_file)) {
            check_true(font_registry.load_ttf("default", font_file, 48U), "Test font should load for 3D text reuse test");
            context.renderer2d.font_registry = &font_registry;

            const auto scene = make_reusable_3d_scene(logo_alpha);
            auto evaluated = scene::evaluate_scene_composition_state(
                scene,
                "png_3d_reuse",
                static_cast<std::int64_t>(0),
                nullptr,
                tachyon::scene::EvaluationVariables{},
                context.media.get());
            check_true(evaluated.has_value(), "Reusable 3D scene should evaluate");
            if (evaluated.has_value()) {
                for (auto& layer : evaluated->layers) {
                    if (layer.type == tachyon::LayerType::Image) {
                        layer.asset_path = logo_alpha.string();
                    }
                }

                std::vector<std::size_t> block_indices;
                std::vector<bool> visible_3d_layers;
                block_indices.reserve(evaluated->layers.size());
                visible_3d_layers.reserve(evaluated->layers.size());
                for (std::size_t i = 0; i < evaluated->layers.size(); ++i) {
                    block_indices.push_back(i);
                    visible_3d_layers.push_back(true);
                }

                tachyon::Scene3DBridgeInput input;
                input.state = &(*evaluated);
                input.block_indices = &block_indices;
                input.visible_3d_layers = &visible_3d_layers;
                input.context = &context.renderer2d;
                tachyon::FrameRenderTask task;
                task.frame_number = 0;
                task.time_seconds = 0.0;
                input.task = &task;

                const auto instances = tachyon::build_instances_3d(input);
                check_true(instances.size() == 3U, "Reusable 3D bridge should build image, text and shape instances");
                if (instances.size() == 3U) {
                    check_true(instances[0].mesh_asset != nullptr, "Image layer should build a 3D mesh");
                    check_true(instances[1].mesh_asset != nullptr, "Text layer should build a 3D mesh");
                    check_true(instances[2].mesh_asset != nullptr, "Shape layer should build a 3D mesh");
                }
            }
        }
    }

    {
        RenderContext left_context;
        RenderContext right_context;

        const auto project_card_x = [&](float z, float camera_x, const std::filesystem::path& path, RenderContext& ctx) {
            const auto scene = make_parallax_layer_scene(path, z, camera_x, "png_3d_parallax_projection");
            auto state = scene::evaluate_scene_composition_state(
                scene,
                "png_3d_parallax_projection",
                static_cast<std::int64_t>(0),
                nullptr,
                tachyon::scene::EvaluationVariables{},
                ctx.media.get());

            check_true(state.has_value(), "Parallax scene should evaluate");
            if (!state.has_value()) {
                return -1.0f;
            }

            const auto& camera = state->camera;
            const auto* card_layer = static_cast<const tachyon::scene::EvaluatedLayerState*>(nullptr);
            for (const auto& layer : state->layers) {
                if (layer.type == tachyon::LayerType::Image) {
                    card_layer = &layer;
                    break;
                }
            }
            if (card_layer == nullptr) {
                return -1.0f;
            }

            const tachyon::math::Vector3 card_position = card_layer->world_matrix.transform_point({0.0f, 0.0f, 0.0f});
            const float depth = std::max(1.0f, card_position.z - camera.position.z);
            return (card_position.x - camera.position.x) / depth;
        };

        const float left_foreground = project_card_x(130.0f, -60.0f, asset_dir / "parallax_red.png", left_context);
        const float left_middle = project_card_x(330.0f, -60.0f, asset_dir / "parallax_green.png", left_context);
        const float left_background = project_card_x(560.0f, -60.0f, asset_dir / "parallax_blue.png", left_context);
        const float right_foreground = project_card_x(130.0f, 60.0f, asset_dir / "parallax_red.png", right_context);
        const float right_middle = project_card_x(330.0f, 60.0f, asset_dir / "parallax_green.png", right_context);
        const float right_background = project_card_x(560.0f, 60.0f, asset_dir / "parallax_blue.png", right_context);

        check_true(left_foreground > -0.999f && left_middle > -0.999f && left_background > -0.999f, "Parallax left projections should be valid");
        check_true(right_foreground > -0.999f && right_middle > -0.999f && right_background > -0.999f, "Parallax right projections should be valid");
        if (left_foreground > -0.999f && left_middle > -0.999f && left_background > -0.999f &&
            right_foreground > -0.999f && right_middle > -0.999f && right_background > -0.999f) {
            const float foreground_shift = std::abs(right_foreground - left_foreground);
            const float middle_shift = std::abs(right_middle - left_middle);
            const float background_shift = std::abs(right_background - left_background);
            check_true(foreground_shift > middle_shift, "Foreground layer should move more than middle layer");
            check_true(middle_shift > background_shift, "Middle layer should move more than background layer");
        }
    }

    std::cout << "PNG 3D validation tests " << (g_failures == 0 ? "PASSED" : "FAILED") << "\n";
    return g_failures == 0;
}
