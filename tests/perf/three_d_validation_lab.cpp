#ifndef TACHYON_TESTS_SOURCE_DIR
#define TACHYON_TESTS_SOURCE_DIR "."
#endif

#ifndef TACHYON_ENABLE_3D
#define TACHYON_ENABLE_3D 1
#endif

#include "tachyon/scene/builder.h"
#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/core/render/aov_buffer.h"
#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/scene3d_bridge.h"
#include "tachyon/renderer2d/evaluated_composition/utilities/composition_utils.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/media/management/media_manager.h"
#include "tachyon/text/fonts/core/font_registry.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/render/render_intent.h"
#include "tachyon/transition_registry.h"
#include "tachyon/presets/effects/effect_manifest.h"
#include "tachyon/presets/text/text_builder.h"
#include "tachyon/presets/text/text_manifest.h"
#include "tachyon/presets/text/text_registry.h"
#include "tachyon/text/layout/layout.h"
#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/text/animation/text_presets.h"

#ifdef TACHYON_ENABLE_3D
#include "tachyon/renderer3d/core/ray_tracer.h"
#include "tachyon/renderer3d/modifiers/modifier3d_registry.h"
#endif

#include <filesystem>
#include <functional>
#include <array>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <limits>
#include <optional>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr const char* kValidationFontName = "SFProDisplayBold";

std::filesystem::path diagnostics_root() {
    if (const char* env_path = std::getenv("TACHYON_3D_LAB_OUTPUT_ROOT")) {
        return std::filesystem::path(env_path) / "3d_validation";
    }
    return std::filesystem::path(TACHYON_TESTS_SOURCE_DIR) / ".." / "output" / "3d_validation";
}

std::filesystem::path find_font_file() {
    const std::filesystem::path candidates[] = {
        "assets/fonts/SFPRODISPLAYBOLD.OTF",
        "fonts/SFPRODISPLAYBOLD.OTF",
        "tests/fixtures/fonts/SFPRODISPLAYBOLD.OTF",
        "build/_deps/harfbuzz-src/test/subset/data/fonts/Roboto-Regular.ttf",
        "_deps/harfbuzz-src/test/subset/data/fonts/Roboto-Regular.ttf"
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}

bool should_run_scene(const char* scene_name) {
    const char* filter = std::getenv("TACHYON_3D_LAB_ONLY");
    if (filter == nullptr || *filter == '\0' || std::string_view(filter) == "all") {
        return true;
    }

    std::string_view requested(filter);
    std::size_t begin = 0;
    while (begin < requested.size()) {
        std::size_t end = requested.find(',', begin);
        if (end == std::string_view::npos) {
            end = requested.size();
        }

        std::string_view token = requested.substr(begin, end - begin);
        while (!token.empty() && token.front() == ' ') token.remove_prefix(1);
        while (!token.empty() && token.back() == ' ') token.remove_suffix(1);
        if (token == scene_name) {
            return true;
        }

        begin = end + 1;
    }

    return false;
}

struct ImageDiff {
    double mean_error{0.0};
    double max_error{0.0};
};

ImageDiff compare_surfaces(const tachyon::renderer2d::SurfaceRGBA& a, const tachyon::renderer2d::SurfaceRGBA& b) {
    ImageDiff diff;
    if (a.width() != b.width() || a.height() != b.height()) {
        diff.mean_error = std::numeric_limits<double>::infinity();
        diff.max_error = std::numeric_limits<double>::infinity();
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

tachyon::renderer2d::SurfaceRGBA resize_surface(
    const tachyon::renderer2d::SurfaceRGBA& src,
    std::uint32_t width,
    std::uint32_t height) {

    tachyon::renderer2d::SurfaceRGBA out(width, height);
    if (width == 0U || height == 0U || src.width() == 0U || src.height() == 0U) {
        return out;
    }

    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::uint32_t src_x = static_cast<std::uint32_t>(
                (static_cast<std::uint64_t>(x) * src.width()) / std::max<std::uint32_t>(1U, width));
            const std::uint32_t src_y = static_cast<std::uint32_t>(
                (static_cast<std::uint64_t>(y) * src.height()) / std::max<std::uint32_t>(1U, height));
            out.set_pixel(
                x,
                y,
                src.get_pixel(
                    std::min(src_x, src.width() - 1U),
                    std::min(src_y, src.height() - 1U)));
        }
    }

    return out;
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

tachyon::LayerSpec make_text_spec(const std::string& text, bool typewriter);

tachyon::SceneSpec make_camera_default_scene() {
    using namespace tachyon::scene;

#ifdef TACHYON_ENABLE_3D
    return Composition("camera_default")
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .background(tachyon::BackgroundSpec::from_string("#0d1117"))
        .clear({10, 12, 18, 255})
        .camera3d_layer("cam", [](LayerBuilder& l) {
            l.transform3d().position(0.0, 0.0, -1216.0).done()
             .camera().type("two_node").poi(0.0, 0.0, 0.0).zoom(877.0).done();
        })
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light().type("ambient")
             .color({255, 255, 255, 255})
             .intensity(0.55).done();
        })
        .light_layer("key", [](LayerBuilder& l) {
            l.position3d(980.0, 720.0, -700.0)
             .rotation3d(-30.0, 35.0, 0.0)
             .light().type("directional")
             .color({255, 252, 245, 255})
             .intensity(1.8).done();
        })
        .layer("floor", [](LayerBuilder& l) {
            l.solid("floor")
             .size(2600, 2600)
             .fill_color({24, 26, 34, 255})
             .position3d(640.0, 560.0, 180.0)
             .rotation3d(90.0, 0.0, 0.0)
             .material().roughness(1.0).metallic(0.0).done();
        })
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .size(520, 292)
             .fill_color({206, 112, 42, 255})
             .position3d(640.0, 360.0, 42.0)
             .material().roughness(0.35).metallic(0.05).done();
        })
        .layer("cube_left", [](LayerBuilder& l) {
            l.solid("cube_left")
             .size(132, 132)
             .fill_color({224, 170, 82, 255})
             .position3d(470.0, 392.0, 18.0)
             .material().roughness(0.4).metallic(0.02).done();
        })
        .layer("cube_right", [](LayerBuilder& l) {
            l.solid("cube_right")
             .size(132, 132)
             .fill_color({224, 170, 82, 255})
             .position3d(852.0, 392.0, 18.0)
             .material().roughness(0.4).metallic(0.02).done();
        })
        .layer("title", [](LayerBuilder& l) {
            l.size(1280.0, 720.0);
            l.text("TILT")
             .font(kValidationFontName)
             .font_size(96.0)
             .box(1280.0f, 720.0f)
             .centerText()
             .color({246, 247, 252, 255})
             .done();
            l.position3d(640.0, 308.0, 120.0)
             .rotation3d(-12.0, 0.0, 0.0)
             .extrude3d(0.18)
             .bevel3d(0.02)
             .material().roughness(0.2).metallic(0.0).emission_strength(4.0).emission_color({246, 247, 252, 255}).done();
        })
        .build_scene();
#else
    return Composition("camera_default")
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .clear({16, 18, 24, 255})
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .size(420, 240)
             .fill_color({208, 120, 52, 255})
             .position(430.0, 240.0);
        })
        .layer("floor", [](LayerBuilder& l) {
            l.solid("floor")
             .size(1280, 80)
             .fill_color({26, 28, 38, 255})
             .position(0.0, 640.0);
        })
        .build_scene();
#endif
}

tachyon::SceneSpec make_camera_fallback_scene() {
    using namespace tachyon::scene;

#ifdef TACHYON_ENABLE_3D
    return Composition("camera_fallback")
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .background(tachyon::BackgroundSpec::from_string("#0d1117"))
        .clear({10, 12, 18, 255})
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light().type("ambient")
             .color({255, 255, 255, 255})
             .intensity(0.55).done();
        })
        .light_layer("key", [](LayerBuilder& l) {
            l.position3d(980.0, 720.0, -700.0)
             .rotation3d(-30.0, 35.0, 0.0)
             .light().type("directional")
             .color({255, 252, 245, 255})
             .intensity(1.8).done();
        })
        .layer("floor", [](LayerBuilder& l) {
            l.solid("floor")
             .size(2600, 2600)
             .fill_color({24, 26, 34, 255})
             .position3d(640.0, 560.0, 180.0)
             .rotation3d(90.0, 0.0, 0.0)
             .material().roughness(1.0).metallic(0.0).done();
        })
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .size(520, 292)
             .fill_color({206, 112, 42, 255})
             .position3d(640.0, 360.0, 42.0)
             .material().roughness(0.35).metallic(0.05).done();
        })
        .layer("cube_left", [](LayerBuilder& l) {
            l.solid("cube_left")
             .size(132, 132)
             .fill_color({224, 170, 82, 255})
             .position3d(470.0, 392.0, 18.0)
             .material().roughness(0.4).metallic(0.02).done();
        })
        .layer("cube_right", [](LayerBuilder& l) {
            l.solid("cube_right")
             .size(132, 132)
             .fill_color({224, 170, 82, 255})
             .position3d(852.0, 392.0, 18.0)
             .material().roughness(0.4).metallic(0.02).done();
        })
        .layer("title", [](LayerBuilder& l) {
            l.size(1280.0, 720.0);
            l.text("TILT")
             .font(kValidationFontName)
             .font_size(96.0)
             .box(1280.0f, 720.0f)
             .centerText()
             .color({246, 247, 252, 255})
             .done();
            l.position3d(640.0, 332.0, 120.0)
             .rotation3d(-12.0, 0.0, 0.0)
             .extrude3d(0.18)
             .bevel3d(0.02)
             .material().roughness(0.2).metallic(0.0).emission_strength(4.0).emission_color({246, 247, 252, 255}).done();
        })
        .build_scene();
#else
    return Composition("camera_fallback")
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .clear({10, 12, 18, 255})
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .size(420, 240)
            .fill_color({42, 140, 210, 255})
             .position(420.0, 220.0);
        })
        .build_scene();
#endif
}

tachyon::SceneSpec make_explicit_camera_scene() {
    using namespace tachyon::scene;

#ifdef TACHYON_ENABLE_3D
    return Composition("camera_explicit")
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .background(tachyon::BackgroundSpec::from_string("#0d1117"))
        .clear({14, 16, 22, 255})
        .camera3d_layer("cam", [](LayerBuilder& l) {
             l.transform3d().position(640.0, 360.0, -1000.0).done()
              .camera().type("two_node").poi(640.0, 360.0, 0.0).zoom(877.0).done();
        })
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light().type("ambient")
             .color({255, 255, 255, 255})
             .intensity(0.8).done();
        })
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .size(380, 220)
             .fill_color({226, 88, 82, 255})
             .position3d(640.0, 360.0, 0.0);
        })
        .build_scene();
#else
    return Composition("camera_explicit")
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .clear({14, 16, 22, 255})
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .size(380, 220)
            .fill_color({226, 88, 82, 255})
             .position(450.0, 210.0);
        })
        .build_scene();
#endif
}

tachyon::SceneSpec make_depth_scene(bool swap_layers) {
    using namespace tachyon::scene;

#ifdef TACHYON_ENABLE_3D
    return Composition(swap_layers ? "depth_swapped" : "depth_front_back")
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .background(tachyon::BackgroundSpec::from_string("#0d1117"))
        .clear({8, 10, 15, 255})
        .camera3d_layer("cam", [](LayerBuilder& l) {
             l.transform3d().position(640.0, 360.0, -1000.0).done()
              .camera().type("two_node").poi(640.0, 360.0, 0.0).zoom(877.0).done();
        })
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light().type("ambient")
             .color({255, 255, 255, 255})
             .intensity(0.9).done();
        })
        .layer("floor", [](LayerBuilder& l) {
            l.solid("floor")
             .size(2800, 2800)
             .fill_color({22, 24, 31, 255})
             .position3d(640.0, 570.0, 180.0)
             .rotation3d(90.0, 0.0, 0.0)
             .material().roughness(1.0).done();
        })
        .layer("red", [swap_layers](LayerBuilder& l) {
            l.solid("red")
             .size(360, 220)
             .fill_color({224, 56, 70, 255})
             .position3d(510.0, 360.0, swap_layers ? 44.0 : -38.0)
             .material().roughness(0.35).done();
        })
        .layer("blue", [swap_layers](LayerBuilder& l) {
            l.solid("blue")
             .size(440, 250)
             .fill_color({64, 118, 235, 255})
             .position3d(700.0, 332.0, swap_layers ? -44.0 : 38.0)
             .material().roughness(0.35).done();
        })
        .build_scene();
#else
    return Composition(swap_layers ? "depth_swapped" : "depth_front_back")
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .clear({9, 10, 15, 255})
        .layer("red", [swap_layers](LayerBuilder& l) {
            l.solid("red")
             .size(460, 260)
             .fill_color({235, 52, 72, 255})
             .position(swap_layers ? 500.0 : 420.0, 215.0);
        })
        .layer("blue", [swap_layers](LayerBuilder& l) {
            l.solid("blue")
             .size(460, 260)
            .fill_color({64, 118, 235, 255})
             .position(swap_layers ? 420.0 : 500.0, 215.0);
        })
        .build_scene();
#endif
}

tachyon::SceneSpec make_emission_scene() {
    using namespace tachyon::scene;

#ifdef TACHYON_ENABLE_3D
    return Composition("emission")
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .background(tachyon::BackgroundSpec::from_string("#000000"))
        .clear({0, 0, 0, 255})
        .camera3d_layer("cam", [](LayerBuilder& l) {
             l.transform3d().position(640.0, 360.0, -1000.0).done()
              .camera().type("two_node").poi(640.0, 360.0, 0.0).zoom(877.0).done();
        })
        .layer("floor", [](LayerBuilder& l) {
            l.solid("floor")
             .size(2800, 2800)
             .fill_color({14, 14, 16, 255})
             .position3d(640.0, 580.0, 210.0)
             .rotation3d(90.0, 0.0, 0.0)
             .material().roughness(1.0).done();
        })
        .layer("glow", [](LayerBuilder& l) {
            l.solid("glow")
             .size(430, 210)
             .fill_color({248, 248, 252, 255})
             .position3d(640.0, 342.0, 0.0)
             .material().roughness(0.1).metallic(0.0).emission_strength(28.0).emission_color({248, 248, 252, 255}).done();
        })
        .build_scene();
#else
    return Composition("emission")
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .clear({6, 6, 8, 255})
        .layer("glow", [](LayerBuilder& l) {
            l.solid("glow")
             .size(360, 180)
            .fill_color({245, 245, 250, 255})
             .position(460.0, 240.0);
        })
        .build_scene();
#endif
}

tachyon::LayerSpec make_text_spec(const std::string& text, bool typewriter) {
    tachyon::LayerSpec spec = tachyon::presets::text::headline(text).build();
    spec.font_id = kValidationFontName;
    spec.font_size = 92.0f;
    spec.fill_color.value = tachyon::ColorSpec{246, 247, 252, 255};
    spec.text_box.mode = tachyon::TextBoxMode::Fixed;
    spec.text_box.width = 1280.0f;
    spec.text_box.height = 720.0f;
    spec.width = 1280;
    spec.height = 720;
    spec.transform.position_x = 640.0;
    spec.transform.position_y = 310.0;
    spec.text_box.horizontal_align = tachyon::HorizontalAlign::Center;
    spec.text_box.vertical_align = tachyon::VerticalAlign::Middle;

    if (typewriter) {
        spec.text_animators.push_back(tachyon::text::make_typewriter_minimal_animator(2.2, false));
    }
    return spec;
}

tachyon::SceneSpec make_text_scene(bool behind_plate, bool typewriter) {
    using namespace tachyon::scene;

#ifdef TACHYON_ENABLE_3D
    const double text_z = behind_plate ? 86.0 : -80.0;

    return Composition(typewriter ? "typewriter_scene" : "text_scene")
        .size(1280, 720)
        .fps(30)
        .duration(6.0)
        .background(tachyon::BackgroundSpec::from_string("#0d1117"))
        .clear({10, 12, 18, 255})
        .camera3d_layer("cam", [](LayerBuilder& l) {
             l.transform3d().position(0.0, 0.0, -1000.0).done()
              .camera().type("two_node").poi(0.0, 0.0, 0.0).zoom(877.0).done();
        })
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light().type("ambient")
             .color({255, 255, 255, 255})
             .intensity(0.55).done();
        })
        .light_layer("key", [](LayerBuilder& l) {
             l.position3d(-840.0, 620.0, -620.0)
              .rotation3d(-28.0, -40.0, 0.0)
              .light().type("directional")
              .color({250, 255, 255, 255})
              .intensity(1.2).done();
        })
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .size(520, 292)
             .fill_color({176, 90, 42, 255})
             .position3d(0.0, 0.0, 42.0)
             .anchor(0.0, 0.0)
             .material().roughness(0.35).metallic(0.04).done();
        })
        .layer("title", [text_z, typewriter](LayerBuilder& l) {
            tachyon::LayerSpec spec = make_text_spec(typewriter ? "TILT TILT TILT" : "TILT", typewriter);
            spec.is_3d = true;
            spec.transform.position_x = 0.0;
            spec.transform.position_y = 0.0;
            l.from_spec(spec);
            l.position3d(0.0, -52.0, text_z)
             .rotation3d(-10.0, 0.0, 0.0)
             .extrude3d(0.18)
             .bevel3d(0.02)
             .material().roughness(0.2).metallic(0.0).emission_strength(4.0).emission_color({246, 247, 252, 255}).done();
        })
        .build_scene();
#else
    return Composition(typewriter ? "typewriter_scene" : "text_scene")
        .size(1280, 720)
        .fps(30)
        .duration(6.0)
        .clear({12, 13, 18, 255})
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .size(540, 300)
             .fill_color({172, 84, 44, 255})
             .position(370.0, 200.0);
        })
        .layer("title", [behind_plate, typewriter](LayerBuilder& l) {
            tachyon::LayerSpec spec = make_text_spec("TILT", typewriter);
            l.from_spec(spec);
            l.position(640.0, 340.0 + (behind_plate ? 24.0 : 0.0));
        })
        .build_scene();
#endif
}

bool save_rendered_surface(
    const tachyon::RasterizedFrame2D& frame,
    const std::filesystem::path& path) {

    if (!frame.surface) {
        std::cerr << "FAIL: missing surface for " << path.string() << '\n';
        return false;
    }

    std::filesystem::create_directories(path.parent_path());
    if (!frame.surface->save_png(path)) {
        std::cerr << "FAIL: unable to save " << path.string() << '\n';
        return false;
    }

    std::cout << "Wrote " << path.string() << '\n';
    return true;
}

bool compare_against_reference(
    const tachyon::renderer2d::SurfaceRGBA& rendered,
    tachyon::media::MediaManager& media_manager,
    const std::filesystem::path& reference_path,
    const std::string& label,
    bool expect_match,
    double mean_threshold,
    double max_threshold) {

    const auto* reference = media_manager.get_image(reference_path, tachyon::media::AlphaMode::Straight, nullptr);
    if (!reference) {
        std::cerr << "FAIL: missing reference PNG for " << label << ": " << reference_path.string() << '\n';
        return false;
    }

    const auto resized_rendered = resize_surface(rendered, reference->width(), reference->height());
    const ImageDiff diff = compare_surfaces(resized_rendered, *reference);
    if (expect_match) {
        if (diff.mean_error > mean_threshold || diff.max_error > max_threshold) {
            std::cerr << "FAIL: " << label << " differs from " << reference_path.string()
                      << " mean=" << diff.mean_error << " max=" << diff.max_error << '\n';
            return false;
        }

        std::cout << "[PASS] " << label << " matches " << reference_path.filename().string()
                  << " mean=" << diff.mean_error << " max=" << diff.max_error << '\n';
        return true;
    }

    if (diff.mean_error <= mean_threshold && diff.max_error <= max_threshold) {
        std::cerr << "FAIL: " << label << " is too similar to " << reference_path.string()
                  << " mean=" << diff.mean_error << " max=" << diff.max_error << '\n';
        return false;
    }

    std::cout << "[PASS] " << label << " differs from " << reference_path.filename().string()
              << " mean=" << diff.mean_error << " max=" << diff.max_error << '\n';
    return true;
}

void force_3d_state(tachyon::scene::EvaluatedCompositionState& state) {
    const float depth = std::max(static_cast<float>(state.width), static_cast<float>(state.height)) * 0.95f;

    // Only set default camera if no camera is available
    if (!state.camera.available) {
        state.camera.available = true;
        state.camera.camera_type = "two_node";
        state.camera.position = {0.0f, 0.0f, -depth};
        state.camera.point_of_interest = {0.0f, 0.0f, 0.0f};
        state.camera.up = {0.0f, 1.0f, 0.0f};
        state.camera.roll = 0.0f;
        state.camera.zoom = 877.0f;
        state.camera.aspect = state.height > 0 ? static_cast<float>(state.width) / static_cast<float>(state.height) : 1.777778f;
        state.camera.fov_y_rad = 2.0f * std::atan(static_cast<float>(state.height) / (2.0f * std::max(877.0f, 1.0f)));
        state.camera.camera.transform.position = state.camera.position;
        state.camera.camera.target_position = state.camera.point_of_interest;
        state.camera.camera.up = state.camera.up;
        state.camera.camera.use_target = true;
        state.camera.camera.focal_length_mm = 35.0f;
        state.camera.camera.fov_y_rad = state.camera.fov_y_rad;
        state.camera.camera.aspect = state.camera.aspect;
    }

    for (auto& layer : state.layers) {
        if (layer.type != tachyon::LayerType::Light &&
            layer.type != tachyon::LayerType::NullLayer &&
            layer.type != tachyon::LayerType::Camera) {
            layer.is_3d = true;
            
            // Fix: 3D meshes (solids, text) are already centered at (0,0) in local space.
            // The world_matrix should simply be the translation to the desired world position.
            if (layer.world_matrix.is_identity()) {
                layer.world_matrix = tachyon::math::Matrix4x4::translation(layer.world_position3);
            }
        }
    }
}



bool render_scene_to_png(
    tachyon::RenderContext& context,
    const std::filesystem::path& out_path,
    const tachyon::SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number,
    const std::filesystem::path* reference_path = nullptr,
    bool expect_match = true,
    double mean_threshold = 3.0,
    double max_threshold = 12.0) {

    std::cerr << "[3D PNG] render " << composition_id << " frame=" << frame_number
              << " -> " << out_path.string() << '\n';

    const auto* composition = find_composition(scene, composition_id);
    if (composition == nullptr) {
        std::cerr << "FAIL: missing composition " << composition_id << '\n';
        return false;
    }

    auto state = tachyon::scene::evaluate_scene_composition_state(
        scene,
        composition_id,
        frame_number,
        nullptr,
        {},
        context.media.get());
    if (!state.has_value()) {
        std::cerr << "FAIL: scene evaluation failed for " << composition_id << '\n';
        return false;
    }

    force_3d_state(*state);

    tachyon::RenderPlan plan;
    plan.job_id = "three_d_validation_" + composition_id + "_" + std::to_string(frame_number);
    plan.composition_target = composition_id;
    plan.scene_spec = &scene;
    plan.frame_range = {frame_number, frame_number};
    plan.output.destination.path = out_path.string();
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
    plan.composition.layer_count = composition->layers.size();
    plan.quality_policy.resolution_scale = 1.0f;

    tachyon::FrameRenderTask task;
    task.frame_number = frame_number;
    task.time_seconds = static_cast<double>(frame_number) / 30.0;

    tachyon::renderer2d::EffectRegistry effect_registry;
    tachyon::TransitionRegistry transition_registry;
    tachyon::presets::EffectManifest manifest;
    tachyon::renderer2d::register_builtin_effects(effect_registry, manifest, transition_registry);
    context.renderer2d.effects = std::shared_ptr<tachyon::renderer2d::EffectHost>(
        tachyon::renderer2d::create_effect_host(effect_registry).release());
    tachyon::render::RenderIntent intent;

#ifdef TACHYON_ENABLE_3D
    {
        static tachyon::renderer3d::Modifier3DRegistry modifier_registry;
        context.renderer2d.modifier_registry = &modifier_registry;
        if (!context.renderer2d.ray_tracer) {
            context.renderer2d.ray_tracer = std::make_shared<tachyon::renderer3d::RayTracer>(
                context.renderer2d.media_manager,
                context.renderer2d.modifier_registry);
        }
        context.ray_tracer = context.renderer2d.ray_tracer;

        std::cerr << "[3D PNG] using ray tracer for " << composition_id << '\n';
        auto rendered = tachyon::render_evaluated_composition_2d(
            *state,
            intent,
            plan,
            task,
            context.renderer2d,
            effect_registry);
        if (!rendered.surface) {
            std::cerr << "FAIL: 3D render produced no surface for " << composition_id << '\n';
            return false;
        }
        if (!save_rendered_surface(rendered, out_path)) {
            return false;
        }
        if (reference_path && context.media) {
            return compare_against_reference(
                *rendered.surface,
                *context.media,
                *reference_path,
                composition_id,
                expect_match,
                mean_threshold,
                max_threshold);
        }
        return true;
    }
#else
    const std::uint32_t width = static_cast<std::uint32_t>(std::max<std::int64_t>(1, composition->width));
    const std::uint32_t height = static_cast<std::uint32_t>(std::max<std::int64_t>(1, composition->height));
    tachyon::renderer2d::SurfaceRGBA canvas(width, height);
    canvas.set_profile(context.renderer2d.cms.working_profile);
    canvas.clear(tachyon::renderer2d::Color::transparent());
    if (state->background_color.a > 0) {
        canvas.clear(tachyon::renderer2d::Color{
            static_cast<float>(state->background_color.r) / 255.0f,
            static_cast<float>(state->background_color.g) / 255.0f,
            static_cast<float>(state->background_color.b) / 255.0f,
            static_cast<float>(state->background_color.a) / 255.0f});
    }

    for (const auto& layer : state->layers) {
        if (!layer.enabled || !layer.active || !layer.visible || layer.id.empty()) {
            continue;
        }

        std::cerr << "[3D PNG]   layer " << layer.id << " type=" << static_cast<int>(layer.type) << '\n';
        auto layer_surface = tachyon::renderer2d::render_layer_surface(
            layer,
            *state,
            intent,
            plan,
            task,
            context.renderer2d);
        if (!layer_surface) {
            std::cerr << "FAIL: render_layer_surface returned null for " << layer.id << '\n';
            continue;
        }

        tachyon::composite_surface(
            canvas,
            *layer_surface,
            0,
            0,
            tachyon::renderer2d::BlendMode::Normal);
    }

    tachyon::RasterizedFrame2D rendered;
    rendered.surface = std::make_shared<tachyon::renderer2d::SurfaceRGBA>(std::move(canvas));
    if (!save_rendered_surface(rendered, out_path)) {
        return false;
    }
    if (reference_path && context.media) {
        return compare_against_reference(
            *rendered.surface,
            *context.media,
            *reference_path,
            composition_id,
            expect_match,
            mean_threshold,
            max_threshold);
    }
    return true;
#endif
}

bool run_three_d_validation_lab() {
    std::filesystem::create_directories(diagnostics_root());
    const std::filesystem::path reference_root = std::filesystem::path(TACHYON_TESTS_SOURCE_DIR) / "..";
    const std::filesystem::path ref1 = reference_root / "ref1.png";
    const std::filesystem::path ref2 = reference_root / "ref2.png";
    const std::filesystem::path ref3 = reference_root / "ref3.png";
    const std::filesystem::path ref4 = reference_root / "ref4.png";
    const std::filesystem::path ref5 = reference_root / "ref5.png";

    tachyon::RenderContext context;
    if (!context.media) {
        context.media = std::make_shared<tachyon::media::MediaManager>();
    }

    context.renderer2d.media_manager = context.media.get();
    context.renderer2d.modifier_registry = nullptr;

    tachyon::text::FontRegistry font_registry;
    const bool needs_text = should_run_scene("text_scene") || should_run_scene("typewriter_scene");
    if (needs_text) {
        const auto font_file = find_font_file();
        if (font_file.empty()) {
            std::cerr << "FAIL: no font fixture found for text PNG generation\n";
            return false;
        }
        if (!font_registry.load_ttf(kValidationFontName, font_file, 96U) || !font_registry.set_default(kValidationFontName)) {
            std::cerr << "FAIL: unable to load default font: " << font_file.string() << '\n';
            return false;
        }
        std::cerr << "[3D PNG] using font " << font_file.string() << '\n';
        context.renderer2d.font_registry = &font_registry;
    }

    bool ok = true;
    // Rigid validation order used for visual regression runs.
    if (should_run_scene("camera_default")) {
        ok = render_scene_to_png(context, diagnostics_root() / "camera_default" / "camera_default.png", make_camera_default_scene(), "camera_default", 0) && ok;
    }
    if (should_run_scene("camera_fallback")) {
        ok = render_scene_to_png(context, diagnostics_root() / "camera_fallback" / "camera_fallback.png", make_camera_fallback_scene(), "camera_fallback", 0) && ok;
    }
    if (should_run_scene("depth_front_back")) {
        ok = render_scene_to_png(context, diagnostics_root() / "depth_front_back" / "front_red_back_blue.png", make_depth_scene(false), "depth_front_back", 0, &ref1, true, 4.0, 18.0) && ok;
    }
    if (should_run_scene("depth_swapped")) {
        ok = render_scene_to_png(context, diagnostics_root() / "depth_swapped" / "front_blue_back_red.png", make_depth_scene(true), "depth_swapped", 0, &ref1, false, 8.0, 24.0) && ok;
    }
    if (should_run_scene("emission")) {
        ok = render_scene_to_png(context, diagnostics_root() / "emission" / "emission_no_lights_20.png", make_emission_scene(), "emission", 0, &ref2, true, 4.0, 18.0) && ok;
    }
    if (should_run_scene("text_scene")) {
        ok = render_scene_to_png(context, diagnostics_root() / "text_scene" / "text_front_of_plate.png", make_text_scene(false, false), "text_scene", 0, &ref3, true, 4.0, 18.0) && ok;
        ok = render_scene_to_png(context, diagnostics_root() / "text_scene" / "text_behind_plate.png", make_text_scene(true, false), "text_scene", 0, &ref4, true, 4.0, 18.0) && ok;
    }
    if (should_run_scene("typewriter_scene")) {
        ok = render_scene_to_png(context, diagnostics_root() / "typewriter_scene" / "frame000.png", make_text_scene(false, true), "typewriter_scene", 0) && ok;
        ok = render_scene_to_png(context, diagnostics_root() / "typewriter_scene" / "frame075.png", make_text_scene(false, true), "typewriter_scene", 75) && ok;
        ok = render_scene_to_png(context, diagnostics_root() / "typewriter_scene" / "frame149.png", make_text_scene(false, true), "typewriter_scene", 149) && ok;

        if (context.media) {
            const auto* frame000 = context.media->get_image(diagnostics_root() / "typewriter_scene" / "frame000.png", tachyon::media::AlphaMode::Straight, nullptr);
            const auto* frame075 = context.media->get_image(diagnostics_root() / "typewriter_scene" / "frame075.png", tachyon::media::AlphaMode::Straight, nullptr);
            const auto* frame149 = context.media->get_image(diagnostics_root() / "typewriter_scene" / "frame149.png", tachyon::media::AlphaMode::Straight, nullptr);
            if (!frame000 || !frame075 || !frame149) {
                std::cerr << "FAIL: unable to reload typewriter validation PNGs\n";
                ok = false;
            } else {
                const ImageDiff early_mid = compare_surfaces(*frame000, *frame075);
                const ImageDiff mid_late = compare_surfaces(*frame075, *frame149);
                if (early_mid.mean_error <= 0.5 && early_mid.max_error <= 2.0) {
                    std::cerr << "FAIL: typewriter_scene frame000 and frame075 are too similar\n";
                    ok = false;
                }
                if (mid_late.mean_error <= 0.5 && mid_late.max_error <= 2.0) {
                    std::cerr << "FAIL: typewriter_scene frame075 and frame149 are too similar\n";
                    ok = false;
                }
            }
        }
    }

    return ok;
}

} // namespace

int main() {
    const bool ok = run_three_d_validation_lab();
    std::fflush(nullptr);
    std::_Exit(ok ? 0 : 1);
}
