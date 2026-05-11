#ifndef TACHYON_ENABLE_3D
#define TACHYON_ENABLE_3D 1
#endif

#include "tachyon/scene/builder.h"
#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
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
#include <iostream>
#include <memory>
#include <optional>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

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
    return filter == nullptr || std::string(filter) == scene_name;
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

tachyon::SceneSpec make_camera_default_scene() {
    using namespace tachyon::scene;

#ifdef TACHYON_ENABLE_3D
    return Composition("camera_default")
        .size(1280, 720)
        .fps(30)
        .duration(1.0)
        .clear({16, 18, 24, 255})
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
             .size(420, 240)
             .fill_color({208, 120, 52, 255})
             .position3d(0.0, 0.0, 0.0);
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
        .clear({10, 12, 18, 255})
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .size(420, 240)
             .fill_color({42, 140, 210, 255})
             .position3d(0.0, 0.0, 0.0);
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
        .clear({14, 16, 22, 255})
        .camera3d_layer("cam", [](LayerBuilder& l) {})
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light().type("ambient")
             .color({255, 255, 255, 255})
             .intensity(0.8).done();
        })
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .size(380, 220)
             .fill_color({226, 88, 82, 255})
             .position3d(0.0, 0.0, 0.0);
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
        .camera3d_layer("cam", [](LayerBuilder& l) {})
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light().type("ambient")
             .color({255, 255, 255, 255})
             .intensity(0.9).done();
        })
        .layer("red", [swap_layers](LayerBuilder& l) {
            l.solid("red")
             .size(460, 260)
             .fill_color({235, 52, 72, 255})
             .position3d(0.0, 0.0, swap_layers ? 50.0 : -50.0);
        })
        .layer("blue", [swap_layers](LayerBuilder& l) {
            l.solid("blue")
             .size(460, 260)
             .fill_color({64, 118, 235, 255})
             .position3d(0.0, 0.0, swap_layers ? -50.0 : 50.0);
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
        .camera3d_layer("cam", [](LayerBuilder& l) {})
        .layer("glow", [](LayerBuilder& l) {
            l.solid("glow")
             .size(360, 180)
             .fill_color({245, 245, 250, 255})
             .position3d(0.0, 0.0, 0.0)
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
        .camera3d_layer("cam", [](LayerBuilder& l) {})
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light().type("ambient")
             .color({255, 255, 255, 255})
             .intensity(0.45).done();
        })
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .size(540, 300)
             .fill_color({172, 84, 44, 255})
             .position3d(0.0, 0.0, 40.0);
        })
        .layer("title", [text_z, typewriter](LayerBuilder& l) {
            tachyon::LayerSpec spec = make_text_spec("TILT", typewriter);
            l.from_spec(spec);
            l.position3d(0.0, 0.0, text_z)
             .rotation3d(-20.0, 0.0, 0.0)
             .extrude3d(0.18)
             .bevel3d(0.02)
             .material().emission_strength(10.0).emission_color({246, 247, 252, 255}).done();
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
            layer.type != tachyon::LayerType::NullLayer) {
            layer.is_3d = true;
        }
    }
}



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
        return save_rendered_surface(rendered, out_path);
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
    return save_rendered_surface(rendered, out_path);
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
    if (needs_text) {
        const auto font_file = find_font_file();
        if (font_file.empty()) {
            std::cerr << "FAIL: no font fixture found for text PNG generation\n";
            return false;
        }
        if (!font_registry.load_ttf("default", font_file, 96U) || !font_registry.set_default("default")) {
            std::cerr << "FAIL: unable to load default font: " << font_file.string() << '\n';
            return false;
        }
        std::cerr << "[3D PNG] using font " << font_file.string() << '\n';
        context.renderer2d.font_registry = &font_registry;
    }

    bool ok = true;
    if (should_run_scene("camera_default")) {
        ok = render_scene_to_png(context, diagnostics_root() / "01_builder" / "camera_default_two_node.png", make_camera_default_scene(), "camera_default", 0) && ok;
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
