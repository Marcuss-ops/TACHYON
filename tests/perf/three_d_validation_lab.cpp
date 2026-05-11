#ifndef TACHYON_TESTS_SOURCE_DIR
#define TACHYON_TESTS_SOURCE_DIR "."
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
#include "tachyon/renderer3d/core/renderer3d_backend_factory.h"
#include "tachyon/renderer3d/modifiers/modifier3d_registry.h"
#endif

#include <filesystem>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <functional>
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

std::filesystem::path diagnostics_root() {
    if (const char* override_root = std::getenv("TACHYON_3D_LAB_OUTPUT_ROOT");
        override_root != nullptr && override_root[0] != '\0') {
        return std::filesystem::path(override_root) / "3d_validation";
    }

    const auto workspace_root = std::filesystem::path(TACHYON_TESTS_SOURCE_DIR).parent_path().parent_path();
    const auto preferred_repo = workspace_root / "Tachyon";
    if (std::filesystem::exists(preferred_repo)) {
        return preferred_repo / "output" / "3d_validation";
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
    return filter == nullptr || std::string(filter) == scene_name;
}

bool is_fast_mode() {
    const char* fast = std::getenv("TACHYON_3D_LAB_FAST");
    return fast != nullptr && fast[0] != '\0' && fast[0] != '0';
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

struct SurfaceMetrics {
    std::uint64_t foreground_pixels{0};
    std::uint64_t opaque_pixels{0};
    double mean_luma{0.0};
    double mean_alpha{0.0};
    double mean_red{0.0};
    double mean_green{0.0};
    double mean_blue{0.0};
    bool has_foreground{false};
    std::uint32_t min_x{0};
    std::uint32_t min_y{0};
    std::uint32_t max_x{0};
    std::uint32_t max_y{0};
};

std::string escape_json(std::string_view value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const char c : value) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                out.push_back(c);
                break;
        }
    }
    return out;
}

SurfaceMetrics analyze_surface(const tachyon::renderer2d::SurfaceRGBA& surface) {
    SurfaceMetrics metrics;
    const std::uint32_t width = surface.width();
    const std::uint32_t height = surface.height();
    if (width == 0U || height == 0U) {
        return metrics;
    }

    bool bbox_initialized = false;
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const auto px = surface.get_pixel(x, y);
            metrics.mean_red += px.r;
            metrics.mean_green += px.g;
            metrics.mean_blue += px.b;
            metrics.mean_alpha += px.a;
            metrics.mean_luma += 0.2126 * static_cast<double>(px.r) +
                                 0.7152 * static_cast<double>(px.g) +
                                 0.0722 * static_cast<double>(px.b);

            if (px.a > 0.01f) {
                ++metrics.foreground_pixels;
                if (!bbox_initialized) {
                    metrics.min_x = metrics.max_x = x;
                    metrics.min_y = metrics.max_y = y;
                    bbox_initialized = true;
                } else {
                    metrics.min_x = std::min(metrics.min_x, x);
                    metrics.min_y = std::min(metrics.min_y, y);
                    metrics.max_x = std::max(metrics.max_x, x);
                    metrics.max_y = std::max(metrics.max_y, y);
                }
            }

            if (px.a > 0.99f) {
                ++metrics.opaque_pixels;
            }
        }
    }

    const double total_pixels = static_cast<double>(width) * static_cast<double>(height);
    if (total_pixels > 0.0) {
        metrics.mean_red /= total_pixels;
        metrics.mean_green /= total_pixels;
        metrics.mean_blue /= total_pixels;
        metrics.mean_alpha /= total_pixels;
        metrics.mean_luma /= total_pixels;
    }
    metrics.has_foreground = bbox_initialized;
    return metrics;
}

bool write_surface_report(
    const tachyon::renderer2d::SurfaceRGBA& surface,
    const std::filesystem::path& png_path,
    const std::string& scene_name,
    std::int64_t frame_number) {

    const auto metrics = analyze_surface(surface);
    std::filesystem::path report_path = png_path;
    report_path.replace_extension(".report.json");
    std::ofstream out(report_path);
    if (!out) {
        std::cerr << "FAIL: unable to write " << report_path.string() << '\n';
        return false;
    }

    const std::uint64_t total_pixels = static_cast<std::uint64_t>(surface.width()) * static_cast<std::uint64_t>(surface.height());
    const double foreground_ratio = total_pixels > 0U
        ? static_cast<double>(metrics.foreground_pixels) / static_cast<double>(total_pixels)
        : 0.0;
    const char* dominant_channel = "r";
    if (metrics.mean_green >= metrics.mean_red && metrics.mean_green >= metrics.mean_blue) {
        dominant_channel = "g";
    } else if (metrics.mean_blue >= metrics.mean_red && metrics.mean_blue >= metrics.mean_green) {
        dominant_channel = "b";
    }
    const bool pass = metrics.has_foreground && metrics.foreground_pixels >= 1000U && foreground_ratio >= 0.0025;

    out << "{\n";
    out << "  \"scene\": \"" << escape_json(scene_name) << "\",\n";
    out << "  \"frame\": " << frame_number << ",\n";
    out << "  \"width\": " << surface.width() << ",\n";
    out << "  \"height\": " << surface.height() << ",\n";
    out << "  \"foreground_pixels\": " << metrics.foreground_pixels << ",\n";
    out << "  \"foreground_ratio\": " << foreground_ratio << ",\n";
    out << "  \"opaque_pixels\": " << metrics.opaque_pixels << ",\n";
    out << "  \"mean_luma\": " << metrics.mean_luma << ",\n";
    out << "  \"mean_alpha\": " << metrics.mean_alpha << ",\n";
    out << "  \"mean_rgb\": {\n";
    out << "    \"r\": " << metrics.mean_red << ",\n";
    out << "    \"g\": " << metrics.mean_green << ",\n";
    out << "    \"b\": " << metrics.mean_blue << "\n";
    out << "  },\n";
    out << "  \"dominant_channel\": \"" << dominant_channel << "\",\n";
    out << "  \"bbox\": ";
    if (metrics.has_foreground) {
        out << "{ \"min_x\": " << metrics.min_x
            << ", \"min_y\": " << metrics.min_y
            << ", \"max_x\": " << metrics.max_x
            << ", \"max_y\": " << metrics.max_y << " },\n";
    } else {
        out << "null,\n";
    }
    out << "  \"status\": \"" << (pass ? "pass" : "fail") << "\"\n";
    out << "}\n";

    if (!out) {
        std::cerr << "FAIL: unable to finalize " << report_path.string() << '\n';
        return false;
    }

    if (!pass) {
        std::cerr << "FAIL: validation metrics weak for " << png_path.string()
                  << " foreground_pixels=" << metrics.foreground_pixels
                  << " foreground_ratio=" << foreground_ratio
                  << '\n';
    }

    std::cout << "Wrote " << report_path.string() << '\n';
    return pass;
}

tachyon::SceneSpec make_camera_default_scene() {
    using namespace tachyon::scene;

#ifdef TACHYON_ENABLE_3D
    return Composition("camera_default")
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .clear({9, 12, 20, 255})
        .camera3d_layer("cam", [](LayerBuilder& l) {
            l.transform3d().position(640.0, 360.0, -1216.0).done()
             .camera().type("two_node").poi(640.0, 360.0, 0.0).zoom(877.0).done();
        })
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light().type("ambient")
             .color({255, 255, 255, 255})
             .intensity(0.4).done();
        })
        .light_layer("key", [](LayerBuilder& l) {
            l.position3d(1200.0, 800.0, -800.0)
             .rotation3d(-35.0, 45.0, 0.0)
             .light().type("directional")
             .color({255, 252, 245, 255})
             .intensity(1.5).done();
        })
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .size(420, 240)
             .fill_color({208, 120, 52, 255})
             .position3d(640.0, 360.0, 0.0)
             .anchor(0.0, 0.0)
             .material().roughness(0.2).metallic(0.2).done();
        })
        .layer("floor", [](LayerBuilder& l) {
            l.solid("floor")
             .size(4000, 4000)
             .fill_color({30, 32, 40, 255})
             .position3d(640.0, 800.0, 1000.0)
             .rotation3d(90.0, 0.0, 0.0)
             .anchor(0.0, 0.0)
             .material().roughness(0.9).done();
        })
        .layer("cube_front", [](LayerBuilder& l) {
            l.solid("cube_front")
             .size(128, 128)
             .fill_color({234, 54, 72, 255})
             .position3d(-320.0, -40.0, -80.0);
        })
        .layer("cube_back", [](LayerBuilder& l) {
            l.solid("cube_back")
             .size(128, 128)
             .fill_color({64, 118, 235, 255})
             .position3d(300.0, -40.0, 80.0);
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
        .clear({9, 12, 20, 255})
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .size(420, 240)
             .fill_color({42, 140, 210, 255})
             .position3d(640.0, 360.0, 0.0);
        })
        .layer("cube_front", [](LayerBuilder& l) {
            l.solid("cube_front")
             .size(128, 128)
             .fill_color({234, 54, 72, 255})
             .position3d(-320.0, -40.0, -80.0);
        })
        .layer("cube_back", [](LayerBuilder& l) {
            l.solid("cube_back")
             .size(128, 128)
             .fill_color({64, 118, 235, 255})
             .position3d(300.0, -40.0, 80.0);
        })
        .build_scene();
#else
    return Composition("camera_fallback")
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .clear({9, 12, 20, 255})
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .size(560, 300)
            .fill_color({212, 116, 52, 255})
             .position(360.0, 180.0);
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
        .clear({9, 10, 15, 255})
        .camera3d_layer("cam", [](LayerBuilder& l) {
             l.transform3d().position(640.0, 360.0, -1000.0).done()
              .camera().type("two_node").poi(640.0, 360.0, 0.0).zoom(877.0).done();
        })
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light().type("ambient")
             .color({255, 255, 255, 255})
             .intensity(0.9).done();
        })
        .layer("red", [swap_layers](LayerBuilder& l) {
            l.solid("red")
             .size(460, 260)
             .fill_color({235, 52, 72, 255})
             .position3d(640.0, 360.0, swap_layers ? 50.0 : -50.0);
        })
        .layer("blue", [swap_layers](LayerBuilder& l) {
            l.solid("blue")
             .size(460, 260)
             .fill_color({64, 118, 235, 255})
             .position3d(640.0, 360.0, swap_layers ? -50.0 : 50.0);
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
        .clear({6, 6, 8, 255})
        .camera3d_layer("cam", [](LayerBuilder& l) {
             l.transform3d().position(640.0, 360.0, -1000.0).done()
              .camera().type("two_node").poi(640.0, 360.0, 0.0).zoom(877.0).done();
        })
        .layer("glow", [](LayerBuilder& l) {
            l.solid("glow")
             .size(360, 180)
             .fill_color({245, 245, 250, 255})
             .position3d(640.0, 360.0, 0.0)
             .material().emission_strength(20.0).emission_color({245, 245, 250, 255}).done();
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
    spec.fill_color.value = tachyon::ColorSpec{246, 247, 252, 255};
    spec.font_id = "default";
    spec.width = 900;
    spec.height = 220;
    spec.transform.position_x = 640.0;
    spec.transform.position_y = 360.0;
    spec.transform.anchor_point.value = tachyon::math::Vector2{0.0f, 0.0f};
    if (typewriter) {
        spec.text_animators.push_back(tachyon::text::make_typewriter_minimal_animator(0.8, false));
    }
    return spec;
}

tachyon::SceneSpec make_text_scene(bool behind_plate, bool typewriter) {
    using namespace tachyon::scene;

#ifdef TACHYON_ENABLE_3D
    const double text_z = behind_plate ? 92.0 : -30.0;

    return Composition(typewriter ? "typewriter_scene" : "text_scene")
        .size(1280, 720)
        .fps(30)
        .duration(6.0)
        .clear({12, 13, 18, 255})
        .camera3d_layer("cam", [](LayerBuilder& l) {
             l.transform3d().position(640.0, 360.0, -1000.0).done()
              .camera().type("two_node").poi(640.0, 360.0, 0.0).zoom(877.0).done();
        })
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light().type("ambient")
             .color({255, 255, 255, 255})
             .intensity(0.4).done();
        })
        .light_layer("key", [](LayerBuilder& l) {
             l.position3d(-800.0, 600.0, -600.0)
              .rotation3d(-30.0, -45.0, 0.0)
              .light().type("directional")
              .color({250, 255, 255, 255})
              .intensity(1.0).done();
        })
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .size(540, 300)
             .fill_color({172, 84, 44, 255})
             .position3d(640.0, 360.0, 40.0)
             .anchor(0.0, 0.0);
        })
        .layer("title", [text_z, typewriter](LayerBuilder& l) {
            tachyon::LayerSpec spec = make_text_spec("TILT", typewriter);
            l.from_spec(spec);
            l.position3d(640.0, 360.0, text_z)
             .rotation3d(-20.0, 0.0, 0.0)
             .anchor(0.0, 0.0)
             .extrude3d(20.0)
             .bevel3d(2.0)
             .material().emission_strength(5.0).emission_color({246, 247, 252, 255}).done();
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

bool save_surface_png(
    const std::shared_ptr<tachyon::renderer2d::SurfaceRGBA>& surface,
    const std::filesystem::path& path) {

    if (!surface) {
        std::cerr << "FAIL: missing surface for " << path.string() << '\n';
        return false;
    }

    std::filesystem::create_directories(path.parent_path());
    if (!surface->save_png(path)) {
        std::cerr << "FAIL: unable to save " << path.string() << '\n';
        return false;
    }

    std::cout << "Wrote " << path.string() << '\n';
    return true;
}

void blit_text_surface(
    tachyon::renderer2d::SurfaceRGBA& dst,
    const tachyon::text::TextRasterSurface& src,
    int x,
    int y) {

    const auto& pixels = src.rgba_pixels();
    const std::uint32_t src_w = src.width();
    const std::uint32_t src_h = src.height();

    for (std::uint32_t sy = 0; sy < src_h; ++sy) {
        for (std::uint32_t sx = 0; sx < src_w; ++sx) {
            const std::size_t idx = (static_cast<std::size_t>(sy) * src_w + sx) * 4U;
            const std::uint8_t a = pixels[idx + 3U];
            if (a == 0U) {
                continue;
            }

            const int dx = x + static_cast<int>(sx);
            const int dy = y + static_cast<int>(sy);
            if (dx < 0 || dy < 0 ||
                dx >= static_cast<int>(dst.width()) ||
                dy >= static_cast<int>(dst.height())) {
                continue;
            }

            dst.blend_pixel(
                static_cast<std::uint32_t>(dx),
                static_cast<std::uint32_t>(dy),
                tachyon::renderer2d::Color{
                    static_cast<float>(pixels[idx + 0U]) / 255.0f,
                    static_cast<float>(pixels[idx + 1U]) / 255.0f,
                    static_cast<float>(pixels[idx + 2U]) / 255.0f,
                    static_cast<float>(a) / 255.0f});
        }
    }
}

void force_3d_state(tachyon::scene::EvaluatedCompositionState& state) {
    const float cx = static_cast<float>(state.width) * 0.5f;
    const float cy = static_cast<float>(state.height) * 0.5f;
    const float depth = std::max(static_cast<float>(state.width), static_cast<float>(state.height)) * 0.95f;

    state.camera.available = true;
    state.camera.camera_type = "two_node";
    state.camera.position = {cx, cy, -depth};
    state.camera.point_of_interest = {cx, cy, 0.0f};
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

void draw_text(
    tachyon::renderer2d::SurfaceRGBA& dst,
    const tachyon::text::Font& font,
    const std::string& text,
    int x,
    int y,
    std::uint32_t pixel_size,
    const tachyon::renderer2d::Color& color) {

    tachyon::text::TextStyle style;
    style.pixel_size = pixel_size;
    style.fill_color = color;

    tachyon::TextBoxSpec box;
    box.mode = tachyon::TextBoxMode::Auto;

    const auto raster = tachyon::text::rasterize_text_rgba(
        font,
        text,
        style,
        box,
        tachyon::text::TextLayoutOptions{});
    blit_text_surface(dst, raster, x, y);
}

void draw_text_lines(
    tachyon::renderer2d::SurfaceRGBA& dst,
    const tachyon::text::Font& font,
    const std::vector<std::string>& lines,
    int x,
    int y,
    std::uint32_t pixel_size,
    const tachyon::renderer2d::Color& color,
    int line_gap = 6) {

    int cy = y;
    for (const auto& line : lines) {
        draw_text(dst, font, line, x, cy, pixel_size, color);
        cy += static_cast<int>(pixel_size) + line_gap;
    }
}

tachyon::renderer2d::Color hsv_to_rgb(float h, float s, float v) {
    const float scaled = h * 6.0f;
    const float i = std::floor(scaled);
    const float f = scaled - i;
    const float p = v * (1.0f - s);
    const float q = v * (1.0f - f * s);
    const float t = v * (1.0f - (1.0f - f) * s);
    switch (static_cast<int>(i) % 6) {
        case 0: return {v, t, p, 1.0f};
        case 1: return {q, v, p, 1.0f};
        case 2: return {p, v, t, 1.0f};
        case 3: return {p, q, v, 1.0f};
        case 4: return {t, p, v, 1.0f};
        default: return {v, p, q, 1.0f};
    }
}

tachyon::renderer2d::Color color_from_id(std::uint32_t id) {
    if (id == 0U) {
        return {0.55f, 0.55f, 0.55f, 1.0f};
    }
    const float hue = std::fmod(static_cast<float>(id) * 0.61803398875f, 1.0f);
    return hsv_to_rgb(hue, 0.65f, 0.95f);
}

tachyon::renderer2d::SurfaceRGBA resize_surface(
    const tachyon::renderer2d::SurfaceRGBA& src,
    std::uint32_t width,
    std::uint32_t height) {

    tachyon::renderer2d::SurfaceRGBA out(width, height);
    out.set_profile(src.profile());
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

std::shared_ptr<tachyon::renderer2d::SurfaceRGBA> surface_from_beauty(
    const tachyon::AOVBuffer& aovs) {

    auto surface = std::make_shared<tachyon::renderer2d::SurfaceRGBA>(aovs.width, aovs.height);
    surface->clear(tachyon::renderer2d::Color{0.02f, 0.02f, 0.03f, 1.0f});
    if (aovs.width == 0U || aovs.height == 0U || aovs.beauty_rgba.empty()) {
        return surface;
    }

    for (std::uint32_t y = 0; y < aovs.height; ++y) {
        for (std::uint32_t x = 0; x < aovs.width; ++x) {
            const std::size_t idx = (static_cast<std::size_t>(y) * aovs.width + x) * 4U;
            surface->set_pixel(
                x,
                y,
                {
                    std::clamp(aovs.beauty_rgba[idx + 0U], 0.0f, 1.0f),
                    std::clamp(aovs.beauty_rgba[idx + 1U], 0.0f, 1.0f),
                    std::clamp(aovs.beauty_rgba[idx + 2U], 0.0f, 1.0f),
                    std::clamp(aovs.beauty_rgba[idx + 3U], 0.0f, 1.0f)});
        }
    }
    return surface;
}

std::shared_ptr<tachyon::renderer2d::SurfaceRGBA> surface_from_depth(
    const tachyon::AOVBuffer& aovs) {

    auto surface = std::make_shared<tachyon::renderer2d::SurfaceRGBA>(aovs.width, aovs.height);
    surface->clear(tachyon::renderer2d::Color{0.02f, 0.02f, 0.03f, 1.0f});
    if (aovs.width == 0U || aovs.height == 0U || aovs.depth_z.empty()) {
        return surface;
    }

    float min_depth = std::numeric_limits<float>::max();
    float max_depth = 0.0f;
    for (float depth : aovs.depth_z) {
        if (!std::isfinite(depth) || depth <= 0.0f || depth >= 1e6f) {
            continue;
        }
        min_depth = std::min(min_depth, depth);
        max_depth = std::max(max_depth, depth);
    }
    if (!std::isfinite(min_depth) || !std::isfinite(max_depth) || max_depth <= min_depth + 1e-3f) {
        min_depth = 0.0f;
        max_depth = 1.0f;
    }

    for (std::uint32_t y = 0; y < aovs.height; ++y) {
        for (std::uint32_t x = 0; x < aovs.width; ++x) {
            const std::size_t idx = static_cast<std::size_t>(y) * aovs.width + x;
            const float depth = aovs.depth_z[idx];
            float t = 0.0f;
            if (std::isfinite(depth) && depth > 0.0f && depth < 1e6f) {
                t = std::clamp((depth - min_depth) / (max_depth - min_depth), 0.0f, 1.0f);
            }
            const float shade = 1.0f - t;
            surface->set_pixel(x, y, {shade, shade, shade, 1.0f});
        }
    }
    return surface;
}

std::shared_ptr<tachyon::renderer2d::SurfaceRGBA> surface_from_normals(
    const tachyon::AOVBuffer& aovs) {

    auto surface = std::make_shared<tachyon::renderer2d::SurfaceRGBA>(aovs.width, aovs.height);
    surface->clear(tachyon::renderer2d::Color{0.02f, 0.02f, 0.03f, 1.0f});
    if (aovs.width == 0U || aovs.height == 0U || aovs.normal_xyz.empty()) {
        return surface;
    }

    for (std::uint32_t y = 0; y < aovs.height; ++y) {
        for (std::uint32_t x = 0; x < aovs.width; ++x) {
            const std::size_t idx = static_cast<std::size_t>(y) * aovs.width + x;
            const std::size_t nidx = idx * 3U;
            const float nx = aovs.normal_xyz[nidx + 0U];
            const float ny = aovs.normal_xyz[nidx + 1U];
            const float nz = aovs.normal_xyz[nidx + 2U];
            surface->set_pixel(
                x,
                y,
                {
                    std::clamp(nx * 0.5f + 0.5f, 0.0f, 1.0f),
                    std::clamp(ny * 0.5f + 0.5f, 0.0f, 1.0f),
                    std::clamp(nz * 0.5f + 0.5f, 0.0f, 1.0f),
                    1.0f});
        }
    }
    return surface;
}

std::shared_ptr<tachyon::renderer2d::SurfaceRGBA> surface_from_ids(
    const std::vector<std::uint32_t>& ids,
    std::uint32_t width,
    std::uint32_t height) {

    auto surface = std::make_shared<tachyon::renderer2d::SurfaceRGBA>(width, height);
    surface->clear(tachyon::renderer2d::Color{0.03f, 0.03f, 0.05f, 1.0f});
    if (width == 0U || height == 0U || ids.empty()) {
        return surface;
    }

    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::size_t idx = static_cast<std::size_t>(y) * width + x;
            surface->set_pixel(x, y, color_from_id(ids[idx]));
        }
    }
    return surface;
}

void draw_frame(
    tachyon::renderer2d::SurfaceRGBA& canvas,
    const tachyon::renderer2d::RectI& frame,
    const tachyon::renderer2d::Color& color) {

    canvas.fill_rect(frame, color, false);
    canvas.fill_rect(
        tachyon::renderer2d::RectI{frame.x, frame.y, frame.width, 2},
        tachyon::renderer2d::Color{color.r * 1.3f, color.g * 1.3f, color.b * 1.3f, 1.0f},
        false);
}

struct AtlasPanel {
    std::string title;
    std::string subtitle;
    std::shared_ptr<tachyon::renderer2d::SurfaceRGBA> surface;
};

void render_panel(
    tachyon::renderer2d::SurfaceRGBA& canvas,
    const tachyon::renderer2d::RectI& rect,
    const AtlasPanel& panel,
    const tachyon::text::Font* font,
    bool large = false) {

    draw_frame(canvas, rect, tachyon::renderer2d::Color{0.12f, 0.13f, 0.17f, 1.0f});
    const int pad = large ? 16 : 12;
    const int inner_x = rect.x + pad;
    const int inner_y = rect.y + pad + 28;
    const int inner_w = rect.width - pad * 2;
    const int inner_h = rect.height - pad * 2 - 32;

    if (panel.surface) {
        const auto fitted = std::make_shared<tachyon::renderer2d::SurfaceRGBA>(
            static_cast<std::uint32_t>(std::max(1, inner_w)),
            static_cast<std::uint32_t>(std::max(1, inner_h)));
        fitted->clear(tachyon::renderer2d::Color{0.03f, 0.03f, 0.05f, 1.0f});
        const auto resized = resize_surface(
            *panel.surface,
            static_cast<std::uint32_t>(std::max(1, inner_w)),
            static_cast<std::uint32_t>(std::max(1, inner_h)));
        fitted->blit(resized, 0, 0);
        canvas.blit(*fitted, inner_x, inner_y);
    }

    canvas.fill_rect(
        tachyon::renderer2d::RectI{rect.x + 8, rect.y + 6, rect.width - 16, 20},
        tachyon::renderer2d::Color{0.04f, 0.05f, 0.07f, 0.75f},
        false);

    if (font != nullptr) {
        draw_text(
            canvas,
            *font,
            panel.title,
            rect.x + 14,
            rect.y + 7,
            large ? 28U : 22U,
            tachyon::renderer2d::Color{1.0f, 0.95f, 0.88f, 1.0f});
        if (!panel.subtitle.empty()) {
            draw_text(
                canvas,
                *font,
                panel.subtitle,
                rect.x + 16,
                rect.y + rect.height - 28,
                large ? 16U : 14U,
                tachyon::renderer2d::Color{0.48f, 0.78f, 1.0f, 1.0f});
        }
    }
}

struct SceneAovBundle {
    tachyon::AOVBuffer aovs;
    std::shared_ptr<tachyon::renderer2d::SurfaceRGBA> beauty;
    std::shared_ptr<tachyon::renderer2d::SurfaceRGBA> depth;
    std::shared_ptr<tachyon::renderer2d::SurfaceRGBA> normal;
    std::shared_ptr<tachyon::renderer2d::SurfaceRGBA> id_pass;
};

std::vector<std::size_t> build_visible_3d_indices(const tachyon::scene::EvaluatedCompositionState& state) {
    std::vector<std::size_t> indices;
    indices.reserve(state.layers.size());
    for (std::size_t i = 0; i < state.layers.size(); ++i) {
        const auto& layer = state.layers[i];
        if (layer.is_3d && layer.visible && layer.enabled && layer.active) {
            indices.push_back(i);
        }
    }
    return indices;
}

std::vector<bool> build_visible_3d_mask(const tachyon::scene::EvaluatedCompositionState& state) {
    std::vector<bool> visible(state.layers.size(), false);
    for (std::size_t i = 0; i < state.layers.size(); ++i) {
        const auto& layer = state.layers[i];
        if (layer.is_3d && layer.visible && layer.enabled && layer.active) {
            visible[i] = true;
        }
    }
    return visible;
}

#ifdef TACHYON_ENABLE_3D
SceneAovBundle render_scene_aovs(
    const tachyon::SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number,
    tachyon::RenderContext& context,
    const std::function<void(tachyon::scene::EvaluatedCompositionState&)>& adjust_state = {}) {

    SceneAovBundle bundle;

    const auto* composition = find_composition(scene, composition_id);
    if (composition == nullptr) {
        std::cerr << "FAIL: missing composition " << composition_id << '\n';
        return bundle;
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
        return bundle;
    }

    if (adjust_state) {
        adjust_state(*state);
    }

    tachyon::RenderPlan plan;
    plan.job_id = "three_d_validation_" + composition_id + "_" + std::to_string(frame_number);
    plan.composition_target = composition_id;
    plan.scene_spec = &scene;
    plan.frame_range = {frame_number, frame_number};
    plan.composition.id = composition->id;
    plan.composition.name = composition->name;
    plan.composition.width = composition->width;
    plan.composition.height = composition->height;
    plan.composition.duration = composition->duration;
    plan.composition.frame_rate = composition->frame_rate;
    plan.composition.background = composition->background;
    plan.quality_policy.resolution_scale = 1.0f;

    tachyon::FrameRenderTask task;
    task.frame_number = frame_number;
    const double fps = plan.composition.frame_rate.value() > 0.0
        ? plan.composition.frame_rate.value()
        : 30.0;
    task.time_seconds = static_cast<double>(frame_number) / fps;
    task.cache_key = {"three_d_validation:" + composition_id + ":" + std::to_string(frame_number)};
    task.cacheable = true;

    tachyon::renderer2d::RenderContext2D& render_context = context.renderer2d;
    static tachyon::renderer3d::Modifier3DRegistry modifier_registry;
    render_context.modifier_registry = &modifier_registry;
    if (!render_context.ray_tracer) {
        render_context.ray_tracer = tachyon::renderer3d::create_renderer3d_backend(
            render_context.media_manager,
            render_context.modifier_registry);
    }
    context.ray_tracer = render_context.ray_tracer;

    const auto visible_3d_layers = build_visible_3d_mask(*state);
    const auto block_indices = build_visible_3d_indices(*state);

    tachyon::Scene3DBridgeInput bridge_input;
    bridge_input.state = &*state;
    bridge_input.plan = &plan;
    bridge_input.task = &task;
    bridge_input.context = &render_context;
    bridge_input.block_indices = &block_indices;
    bridge_input.visible_3d_layers = &visible_3d_layers;

    const auto bridge_output = tachyon::build_evaluated_scene_3d(bridge_input);
    bundle.aovs.resize(static_cast<std::uint32_t>(composition->width), static_cast<std::uint32_t>(composition->height));
    if (render_context.ray_tracer) {
        render_context.ray_tracer->set_samples_per_pixel(1);
        render_context.ray_tracer->set_denoiser_enabled(false);
        render_context.ray_tracer->build_scene(bridge_output.scene3d);
        render_context.ray_tracer->render(
            bridge_output.scene3d,
            bundle.aovs,
            task.time_seconds,
            fps > 0.0 ? 1.0 / fps : 0.0);
    }

    bundle.beauty = surface_from_beauty(bundle.aovs);
    bundle.depth = surface_from_depth(bundle.aovs);
    bundle.normal = surface_from_normals(bundle.aovs);
    bundle.id_pass = surface_from_ids(bundle.aovs.object_id, bundle.aovs.width, bundle.aovs.height);
    return bundle;
}
#endif

void set_camera_zoom(tachyon::scene::EvaluatedCompositionState& state, float zoom) {
    state.camera.zoom = zoom;
    state.camera.fov_y_rad = 2.0f * std::atan(static_cast<float>(state.height) / (2.0f * std::max(zoom, 1.0f)));
    state.camera.camera.fov_y_rad = state.camera.fov_y_rad;
    state.camera.camera.focal_length_mm = std::max(1.0f, zoom / 25.0f);
    state.camera.camera.aspect = state.height > 0
        ? static_cast<float>(state.width) / static_cast<float>(state.height)
        : 1.777778f;
}

std::shared_ptr<tachyon::renderer2d::SurfaceRGBA> make_top_view_panel() {
    auto surface = std::make_shared<tachyon::renderer2d::SurfaceRGBA>(640U, 360U);
    surface->clear(tachyon::renderer2d::Color{0.04f, 0.05f, 0.07f, 1.0f});

    for (std::uint32_t y = 0; y < surface->height(); ++y) {
        for (std::uint32_t x = 0; x < surface->width(); ++x) {
            if ((x % 40U) == 0U || (y % 40U) == 0U) {
                surface->set_pixel(x, y, tachyon::renderer2d::Color{0.12f, 0.14f, 0.19f, 1.0f});
            }
        }
    }

    surface->fill_rect({120, 146, 320, 26}, {0.86f, 0.50f, 0.22f, 1.0f}, false);
    surface->fill_rect({56, 154, 54, 54}, {0.92f, 0.24f, 0.22f, 1.0f}, false);
    surface->fill_rect({474, 146, 54, 54}, {0.22f, 0.40f, 0.92f, 1.0f}, false);
    surface->fill_rect({28, 28, 12, 38}, {0.15f, 0.86f, 0.44f, 1.0f}, false);
    surface->fill_rect({28, 54, 38, 12}, {0.15f, 0.86f, 0.44f, 1.0f}, false);
    return surface;
}

void save_reference_atlas(
    const SceneAovBundle& main_scene,
    const SceneAovBundle& fallback_scene,
    const SceneAovBundle& zoom_300_scene,
    const SceneAovBundle& zoom_1500_scene,
    const tachyon::text::Font* font,
    const std::filesystem::path& path) {

    tachyon::renderer2d::SurfaceRGBA canvas(1664U, 1024U);
    canvas.clear(tachyon::renderer2d::Color{0.03f, 0.04f, 0.06f, 1.0f});

    canvas.fill_rect({0, 0, 1664, 1024}, {0.03f, 0.04f, 0.06f, 1.0f}, false);
    canvas.fill_rect({0, 0, 1664, 3}, {0.15f, 0.18f, 0.26f, 1.0f}, false);

    if (font != nullptr) {
        draw_text(canvas, *font, "CAMERA DEFAULT TWO_NODE", 22, 14, 30U, {1.0f, 1.0f, 1.0f, 1.0f});
        draw_text(canvas, *font, "camera_default_two_node.png", 22, 48, 18U, {0.22f, 0.68f, 0.98f, 1.0f});
        draw_text(canvas, *font, "DEPTH PASS", 1176, 14, 24U, {1.0f, 1.0f, 1.0f, 1.0f});
        draw_text(canvas, *font, "NORMAL PASS", 1176, 208, 24U, {1.0f, 1.0f, 1.0f, 1.0f});
        draw_text(canvas, *font, "MATERIAL ID PASS", 1176, 402, 24U, {1.0f, 1.0f, 1.0f, 1.0f});
        draw_text(canvas, *font, "CAMERA FALLBACK (NO CAMERA LAYER)", 22, 676, 20U, {1.0f, 1.0f, 1.0f, 1.0f});
        draw_text(canvas, *font, "ZOOM 300", 440, 676, 20U, {1.0f, 1.0f, 1.0f, 1.0f});
        draw_text(canvas, *font, "ZOOM 1500", 833, 676, 20U, {1.0f, 1.0f, 1.0f, 1.0f});
        draw_text(canvas, *font, "TOP VIEW (DEBUG)", 1225, 676, 20U, {1.0f, 1.0f, 1.0f, 1.0f});
    }

    render_panel(
        canvas,
        {20, 78, 1132, 570},
        {"MAIN BEAUTY", "camera z=-980, poi z=0, cube front/back, plate orange", main_scene.beauty},
        font,
        true);
    render_panel(
        canvas,
        {1170, 78, 474, 180},
        {"DEPTH PASS", "near bright, far dark", main_scene.depth},
        font);
    render_panel(
        canvas,
        {1170, 272, 474, 180},
        {"NORMAL PASS", "world normals mapped to RGB", main_scene.normal},
        font);
    render_panel(
        canvas,
        {1170, 466, 474, 180},
        {"MATERIAL ID PASS", "instance ids mapped to palette", main_scene.id_pass},
        font);

    render_panel(
        canvas,
        {20, 734, 386, 180},
        {"CAMERA FALLBACK", "no camera layer, default 3D fallback", fallback_scene.beauty},
        font);
    render_panel(
        canvas,
        {418, 734, 386, 180},
        {"ZOOM 300", "wide framing", zoom_300_scene.beauty},
        font);
    render_panel(
        canvas,
        {816, 734, 386, 180},
        {"ZOOM 1500", "tight framing", zoom_1500_scene.beauty},
        font);
    render_panel(
        canvas,
        {1214, 734, 430, 180},
        {"TOP VIEW (DEBUG)", "object layout from above", make_top_view_panel()},
        font);

    if (font != nullptr) {
        draw_text_lines(
            canvas,
            *font,
            {
                "SCENA",
                "- Plate arancione al centro (z = 0)",
                "- Cubo rosso davanti (z < 0)",
                "- Cubo blu dietro (z > 0)",
                "- Griglia prospettica e camera two_node"
            },
            28,
            932,
            16U,
            {1.0f, 0.75f, 0.28f, 1.0f},
            3);
        draw_text_lines(
            canvas,
            *font,
            {
                "COSA VERIFICARE",
                "Perspettiva corretta",
                "Depth buffer coerente",
                "Colori/materiali non grigi",
                "Nessun layer sopra lo Z sbagliato"
            },
            438,
            932,
            16U,
            {0.34f, 0.98f, 0.58f, 1.0f},
            3);
        draw_text_lines(
            canvas,
            *font,
            {
                "PARAMETRI CAMERA ATTESI",
                "type: two_node",
                "position: (0, -120, -980)",
                "poi: (0, 70, 0)",
                "zoom: 877"
            },
            824,
            932,
            16U,
            {0.49f, 0.78f, 1.0f, 1.0f},
            3);
        draw_text_lines(
            canvas,
            *font,
            {
                "RISULTATO ATTESO",
                "Scena 3D pulita, centrata",
                "profondita percepibile",
                "colori e griglia leggibili",
                "panelli diagnostici coerenti"
            },
            1214,
            932,
            16U,
            {1.0f, 0.73f, 0.34f, 1.0f},
            3);
    }

    auto atlas = std::make_shared<tachyon::renderer2d::SurfaceRGBA>(std::move(canvas));
    save_surface_png(atlas, path);
}

#ifdef TACHYON_ENABLE_3D
bool write_reference_atlas(
    tachyon::RenderContext& context,
    const tachyon::text::Font* font,
    const std::filesystem::path& root) {

    const auto main_scene = render_scene_aovs(make_camera_default_scene(), "camera_default", 0, context);
    const auto fallback_scene = render_scene_aovs(make_camera_fallback_scene(), "camera_fallback", 0, context);
    const auto zoom_300_scene = render_scene_aovs(
        make_camera_default_scene(),
        "camera_default",
        0,
        context,
        [](tachyon::scene::EvaluatedCompositionState& state) {
            set_camera_zoom(state, 300.0f);
        });
    const auto zoom_1500_scene = render_scene_aovs(
        make_camera_default_scene(),
        "camera_default",
        0,
        context,
        [](tachyon::scene::EvaluatedCompositionState& state) {
            set_camera_zoom(state, 1500.0f);
        });

    if (!main_scene.beauty || !main_scene.depth || !main_scene.normal || !main_scene.id_pass ||
        !fallback_scene.beauty || !zoom_300_scene.beauty || !zoom_1500_scene.beauty) {
        std::cerr << "FAIL: one or more atlas panels failed to render\n";
        return false;
    }

    const auto builder_root = root / "01_builder";
    const auto camera_root = root / "02_camera";
    const auto debug_root = root / "09_coordinate_system";

    bool report_ok = true;

    report_ok = save_surface_png(main_scene.beauty, builder_root / "camera_default_two_node_beauty.png") && report_ok;
    report_ok = save_surface_png(main_scene.depth, builder_root / "camera_default_two_node_depth.png") && report_ok;
    report_ok = save_surface_png(main_scene.normal, builder_root / "camera_default_two_node_normal.png") && report_ok;
    report_ok = save_surface_png(main_scene.id_pass, builder_root / "camera_default_two_node_material_id.png") && report_ok;
    report_ok = write_surface_report(*main_scene.beauty, builder_root / "camera_default_two_node_beauty.png", "camera_default", 0) && report_ok;
    report_ok = save_surface_png(fallback_scene.beauty, camera_root / "camera_fallback_no_camera.png") && report_ok;
    report_ok = write_surface_report(*fallback_scene.beauty, camera_root / "camera_fallback_no_camera.png", "camera_fallback", 0) && report_ok;
    report_ok = save_surface_png(zoom_300_scene.beauty, camera_root / "zoom_300.png") && report_ok;
    report_ok = write_surface_report(*zoom_300_scene.beauty, camera_root / "zoom_300.png", "camera_default_zoom_300", 0) && report_ok;
    report_ok = save_surface_png(zoom_1500_scene.beauty, camera_root / "zoom_1500.png") && report_ok;
    report_ok = write_surface_report(*zoom_1500_scene.beauty, camera_root / "zoom_1500.png", "camera_default_zoom_1500", 0) && report_ok;
    report_ok = save_surface_png(make_top_view_panel(), debug_root / "top_view_debug.png") && report_ok;

    save_reference_atlas(
        main_scene,
        fallback_scene,
        zoom_300_scene,
        zoom_1500_scene,
        font,
        builder_root / "camera_default_two_node.png");
    return report_ok;
}
#endif

bool render_scene_to_png(
    tachyon::RenderContext& context,
    const std::filesystem::path& out_path,
    const tachyon::SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number) {

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
    const double fps = plan.composition.frame_rate.value() > 0.0
        ? plan.composition.frame_rate.value()
        : 30.0;
    task.time_seconds = static_cast<double>(frame_number) / fps;

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
            context.renderer2d.ray_tracer = tachyon::renderer3d::create_renderer3d_backend(
                context.renderer2d.media_manager,
                context.renderer2d.modifier_registry);
        }
        context.ray_tracer = context.renderer2d.ray_tracer;

        if (context.renderer2d.ray_tracer) {
            std::cerr << "[3D PNG] using ray tracer for " << composition_id << '\n';
        } else {
            std::cerr << "[3D PNG] using 2D fallback preview for " << composition_id << '\n';
        }
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
        const bool saved = save_rendered_surface(rendered, out_path);
        const bool reported = saved && write_surface_report(*rendered.surface, out_path, composition_id, frame_number);
        return saved && reported;
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
    const bool saved = save_rendered_surface(rendered, out_path);
    const bool reported = saved && write_surface_report(*rendered.surface, out_path, composition_id, frame_number);
    return saved && reported;
#endif
}

bool run_three_d_validation_lab() {
    std::filesystem::create_directories(diagnostics_root());

    tachyon::RenderContext context;
    if (!context.media) {
        context.media = std::make_shared<tachyon::media::MediaManager>();
    }

    context.renderer2d.media_manager = context.media.get();
    context.renderer2d.modifier_registry = nullptr;

    tachyon::text::FontRegistry font_registry;
    const bool needs_text = should_run_scene("text_scene") || should_run_scene("typewriter_scene");
    const auto font_file = find_font_file();
    if (!font_file.empty()) {
        if (font_registry.load_ttf("default", font_file, 96U) && font_registry.set_default("default")) {
            std::cerr << "[3D PNG] using font " << font_file.string() << '\n';
            context.renderer2d.font_registry = &font_registry;
        } else if (needs_text) {
            std::cerr << "FAIL: unable to load default font: " << font_file.string() << '\n';
            return false;
        }
    } else if (needs_text) {
        std::cerr << "FAIL: no font fixture found for text PNG generation\n";
        return false;
    }

    bool ok = true;
    if (should_run_scene("camera_default")) {
#ifdef TACHYON_ENABLE_3D
        if (is_fast_mode() || !context.ray_tracer) {
            ok = render_scene_to_png(
                context,
                diagnostics_root() / "01_builder" / "camera_default_two_node.png",
                make_camera_default_scene(),
                "camera_default",
                0) && ok;
        } else {
            ok = write_reference_atlas(context, font_registry.default_font(), diagnostics_root()) && ok;
        }
#else
        ok = render_scene_to_png(
            context,
            diagnostics_root() / "01_builder" / "camera_default_two_node.png",
            make_camera_default_scene(),
            "camera_default",
            0) && ok;
#endif
    }
    if (should_run_scene("camera_fallback")) {
        ok = render_scene_to_png(context, diagnostics_root() / "02_camera" / "camera_fallback_no_camera.png", make_camera_fallback_scene(), "camera_fallback", 0) && ok;
    }
    if (should_run_scene("camera_explicit")) {
        ok = render_scene_to_png(context, diagnostics_root() / "02_camera" / "camera_explicit_front.png", make_explicit_camera_scene(), "camera_explicit", 0) && ok;
    }
    if (should_run_scene("depth_front_back")) {
        ok = render_scene_to_png(context, diagnostics_root() / "03_depth" / "front_red_back_blue.png", make_depth_scene(false), "depth_front_back", 0) && ok;
    }
    if (should_run_scene("depth_swapped")) {
        ok = render_scene_to_png(context, diagnostics_root() / "03_depth" / "front_blue_back_red.png", make_depth_scene(true), "depth_swapped", 0) && ok;
    }
    if (should_run_scene("emission")) {
        ok = render_scene_to_png(context, diagnostics_root() / "05_materials" / "emission_no_lights_20.png", make_emission_scene(), "emission", 0) && ok;
    }
    if (should_run_scene("text_scene")) {
        ok = render_scene_to_png(context, diagnostics_root() / "01_builder" / "from_spec_then_3d.png", make_text_scene(false, false), "text_scene", 0) && ok;
        ok = render_scene_to_png(context, diagnostics_root() / "06_text3d" / "text_front_of_plate.png", make_text_scene(false, false), "text_scene", 0) && ok;
        ok = render_scene_to_png(context, diagnostics_root() / "06_text3d" / "text_behind_plate.png", make_text_scene(true, false), "text_scene", 0) && ok;
    }
    if (should_run_scene("typewriter_scene")) {
        ok = render_scene_to_png(context, diagnostics_root() / "10_typewriter_regression" / "typewriter_11_frame000.png", make_text_scene(false, true), "typewriter_scene", 0) && ok;
        ok = render_scene_to_png(context, diagnostics_root() / "10_typewriter_regression" / "typewriter_11_frame075.png", make_text_scene(false, true), "typewriter_scene", 75) && ok;
        ok = render_scene_to_png(context, diagnostics_root() / "10_typewriter_regression" / "typewriter_11_frame149.png", make_text_scene(false, true), "typewriter_scene", 149) && ok;
    }

    return ok;
}

} // namespace

int main() {
    const bool ok = run_three_d_validation_lab();
    std::fflush(nullptr);
    std::_Exit(ok ? 0 : 1);
}
