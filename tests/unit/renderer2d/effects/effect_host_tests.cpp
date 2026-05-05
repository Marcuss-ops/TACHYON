#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/renderer2d/raster/rasterizer.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

static int g_failures = 0;

static void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_effect_host_tests() {
    using namespace tachyon;
    using namespace tachyon::renderer2d;

    SurfaceRGBA source(16, 16);
    source.clear(Color::transparent());
    source.set_pixel(8, 8, Color::white());

    auto host_ptr = create_effect_host();
    EffectHost& host = *host_ptr;
    EffectHost::register_builtins(host);

    check_true(host.has_effect("tachyon.effect.blur.gaussian"), "Registered gaussian_blur effect");
    check_true(host.has_effect("tachyon.effect.shadow.drop"), "Registered drop_shadow effect");
    check_true(host.has_effect("tachyon.effect.glow"), "Registered glow effect");
    check_true(host.has_effect("tachyon.effect.color.levels"), "Registered levels effect");
    check_true(host.has_effect("tachyon.effect.color.curves"), "Registered curves effect");
    check_true(host.has_effect("tachyon.effect.color.fill"), "Registered fill effect");
    check_true(host.has_effect("tachyon.effect.color.tint"), "Registered tint effect");
    check_true(host.has_effect("tachyon.effect.color.hue_saturation"), "Registered hue_saturation effect");
    check_true(host.has_effect("tachyon.effect.generators.particle_emitter"), "Registered particle_emitter effect");

    RenderContext context;
    context.effects = create_effect_host();
    EffectHost::register_builtins(*context.effects);
    
    check_true(context.effects->has_effect("tachyon.effect.blur.gaussian"), "RenderContext auto-registers gaussian_blur");
    check_true(context.effects->has_effect("tachyon.effect.color.hue_saturation"), "RenderContext auto-registers hue_saturation");

    EffectParams blur_params;
    blur_params.scalars["blur_radius"] = 1.5f;
    const auto blurred_res = host.apply("tachyon.effect.blur.gaussian", source, blur_params);
    check_true(blurred_res.ok(), "Blur application successful");
    const SurfaceRGBA& blurred = *blurred_res.value;
    check_true(blurred.get_pixel(8, 8).a > 0, "Blur keeps center visible");
    check_true(blurred.get_pixel(9, 8).a > 0, "Blur spreads to neighboring pixel");

    EffectParams hue_params;
    hue_params.scalars["hue"] = 180.0f;
    const auto hued_res = host.apply("tachyon.effect.color.hue_saturation", source, hue_params);
    check_true(hued_res.ok(), "Hue application successful");
    const SurfaceRGBA& hued = *hued_res.value;
    check_true(hued.get_pixel(8, 8).a == source.get_pixel(8, 8).a, "HueSaturation preserves alpha");

    EffectParams particle_params;
    particle_params.scalars["seed"] = 1234.0f;
    particle_params.scalars["count"] = 32.0f;
    particle_params.scalars["time"] = 0.5f;
    particle_params.scalars["lifetime"] = 2.0f;
    particle_params.scalars["speed"] = 20.0f;
    particle_params.scalars["radius_min"] = 1.0f;
    particle_params.scalars["radius_max"] = 2.0f;
    particle_params.colors["color"] = Color{1.0f, 180.0f/255.0f, 64.0f/255.0f, 200.0f/255.0f};
    const auto particles_a_res = host.apply("tachyon.effect.generators.particle_emitter", source, particle_params);
    const auto particles_b_res = host.apply("tachyon.effect.generators.particle_emitter", source, particle_params);
    check_true(particles_a_res.ok(), "Particle A successful");
    const SurfaceRGBA& particles_a = *particles_a_res.value;
    const SurfaceRGBA& particles_b = *particles_b_res.value;
    bool has_visible_particle = false;
    for (std::uint32_t y = 0; y < particles_a.height(); ++y) {
        for (std::uint32_t x = 0; x < particles_a.width(); ++x) {
            if (particles_a.get_pixel(x, y).a > 0.0f) {
                has_visible_particle = true;
                break;
            }
        }
        if (has_visible_particle) {
            break;
        }
    }
    bool particle_frames_match = true;
    for (std::uint32_t y = 0; y < particles_a.height() && particle_frames_match; ++y) {
        for (std::uint32_t x = 0; x < particles_a.width(); ++x) {
            if (particles_a.get_pixel(x, y).a != particles_b.get_pixel(x, y).a) {
                particle_frames_match = false;
                break;
            }
        }
    }
    check_true(has_visible_particle, "Particle emitter draws visible particles");
    check_true(particle_frames_match, "Particle emitter is deterministic for fixed seed");

    EffectParams fill_params;
    fill_params.colors["color"] = Color{200.0f/255.0f, 100.0f/255.0f, 50.0f/255.0f, 128.0f/255.0f};
    const auto filled_res = host.apply("tachyon.effect.color.fill", source, fill_params);
    check_true(filled_res.ok(), "Fill successful");
    const SurfaceRGBA& filled = *filled_res.value;
    check_true(filled.get_pixel(8, 8).a >= 0.5f, "Fill correctly applies alpha");
    const Color p = filled.get_pixel(8, 8);
    check_true(std::abs(p.r - 200.0f/255.0f) < 0.001f && std::abs(p.g - 100.0f/255.0f) < 0.001f && std::abs(p.b - 50.0f/255.0f) < 0.001f, "Fill preserves straight RGB");

    SurfaceRGBA transition_from(8, 8);
    transition_from.clear(Color::red());
    SurfaceRGBA transition_to(8, 8);
    transition_to.clear(Color::blue());

    EffectParams transition_params;
    transition_params.scalars["t"] = 1.0f;
    transition_params.strings["transition_id"] = "tachyon.transition.crossfade";
    transition_params.strings["shader_path"] = "studio/library/animations/transitions/crossfade/v1.glsl";
    transition_params.aux_surfaces["transition_to"] = &transition_to;
    const auto transitioned_res = host.apply("tachyon.effect.transition.glsl", transition_from, transition_params);
    check_true(transitioned_res.ok(), "Transition successful");
    const SurfaceRGBA& transitioned = *transitioned_res.value;
    const Color transition_center = transitioned.get_pixel(4, 4);
    check_true(transition_center.b > 0.8f, "Glsl transition reaches the target surface");
    check_true(transition_center.r < 0.3f, "Glsl transition reduces the source surface when t=1");

    EffectParams slide_up_params;
    slide_up_params.scalars["t"] = 1.0f;
    slide_up_params.strings["transition_id"] = "tachyon.transition.slide_up";
    slide_up_params.aux_surfaces["transition_to"] = &transition_to;
    const auto slide_up_res = host.apply("tachyon.effect.transition.glsl", transition_from, slide_up_params);
    check_true(slide_up_res.ok(), "Slide up transition successful");
    const SurfaceRGBA& slide_up = *slide_up_res.value;
    check_true(slide_up.get_pixel(4, 4).b > 0.8f, "Slide up transition reaches the target surface");

    EffectParams swipe_left_params;
    swipe_left_params.scalars["t"] = 1.0f;
    swipe_left_params.strings["transition_id"] = "tachyon.transition.swipe_left";
    swipe_left_params.aux_surfaces["transition_to"] = &transition_to;
    const auto swipe_left_res = host.apply("tachyon.effect.transition.glsl", transition_from, swipe_left_params);
    check_true(swipe_left_res.ok(), "Swipe left transition successful");
    const SurfaceRGBA& swipe_left = *swipe_left_res.value;
    check_true(swipe_left.get_pixel(4, 4).b > 0.8f, "Swipe left transition reaches the target surface");

    PrecompCache cache;
    cache.set_max_bytes(16);

    auto frame_a = std::make_shared<SurfaceRGBA>(1, 1);
    frame_a->set_pixel(0, 0, Color::red());

    auto frame_b = std::make_shared<SurfaceRGBA>(1, 1);
    frame_b->set_pixel(0, 0, Color::green());

    auto frame_c = std::make_shared<SurfaceRGBA>(1, 1);
    frame_c->set_pixel(0, 0, Color::blue());

    cache.store("a", frame_a);
    cache.store("b", frame_b);
    
    check_true(cache.lookup("a") != nullptr, "Cache returns first entry");
    
    cache.store("c", frame_c);
    check_true(cache.lookup("a") != nullptr, "Entry A survives");
    check_true(cache.lookup("b") != nullptr, "Entry B survives");
    
    cache.set_max_bytes(10);
    cache.clear();
    cache.set_max_bytes(10);
    cache.store("a", frame_a); // [a]
    cache.store("b", frame_b); // [b, a]
    check_true(cache.lookup("a") != nullptr, "Lookup A"); // [a, b]
    cache.store("c", frame_c); // [c, a] -> evicts b
    
    check_true(cache.lookup("a") != nullptr, "A survives as it was used recently");
    check_true(cache.lookup("b") == nullptr, "B is evicted as it was LRU");
    check_true(cache.lookup("c") != nullptr, "C is present");

    // Test new transitions: light_leak, film_burn, flash
    EffectParams leak_params;
    leak_params.scalars["t"] = 0.5f;
    leak_params.strings["transition_id"] = "tachyon.transition.light_leak";
    leak_params.aux_surfaces["transition_to"] = &transition_to;
    const auto leaked_res = host.apply("tachyon.effect.transition.glsl", transition_from, leak_params);
    check_true(leaked_res.ok(), "Leaked successful");
    const SurfaceRGBA& leaked = *leaked_res.value;
    check_true(leaked.get_pixel(4, 4).a > 0, "Light leak transition produces visible output");

    EffectParams leak_overlay_params;
    leak_overlay_params.scalars["t"] = 0.5f;
    leak_overlay_params.strings["transition_id"] = "tachyon.transition.light_leak";
    const auto leak_overlay_res = host.apply("tachyon.effect.transition.glsl", SurfaceRGBA(8, 8), leak_overlay_params);
    check_true(leak_overlay_res.ok(), "Leak overlay successful");
    const SurfaceRGBA& leak_overlay = *leak_overlay_res.value;
    const Color leak_overlay_center = leak_overlay.get_pixel(4, 4);
    check_true(leak_overlay_center.r > 0.1f || leak_overlay_center.g > 0.05f || leak_overlay_center.b > 0.01f,
        "Light leak overlay mode produces visible amber output on black");

    EffectParams burn_params;
    burn_params.scalars["t"] = 0.5f;
    burn_params.strings["transition_id"] = "tachyon.transition.film_burn";
    burn_params.aux_surfaces["transition_to"] = &transition_to;
    const auto burned_res = host.apply("tachyon.effect.transition.glsl", transition_from, burn_params);
    check_true(burned_res.ok(), "Burn successful");
    const SurfaceRGBA& burned = *burned_res.value;
    check_true(burned.get_pixel(4, 4).a > 0, "Film burn transition produces visible output");

    EffectParams flash_params;
    flash_params.scalars["t"] = 0.5f;
    flash_params.strings["transition_id"] = "tachyon.transition.flash";
    flash_params.aux_surfaces["transition_to"] = &transition_to;
    const auto flashed_res = host.apply("tachyon.effect.transition.glsl", transition_from, flash_params);
    check_true(flashed_res.ok(), "Flash successful");
    const SurfaceRGBA& flashed = *flashed_res.value;
    check_true(flashed.get_pixel(4, 4).r > 0.5f && flashed.get_pixel(4, 4).g > 0.5f && flashed.get_pixel(4, 4).b > 0.5f, "Flash transition produces bright output at t=0.5");

    return g_failures == 0;
}
