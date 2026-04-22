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

    check_true(host.has_effect("gaussian_blur"), "Registered gaussian_blur effect");
    check_true(host.has_effect("drop_shadow"), "Registered drop_shadow effect");
    check_true(host.has_effect("glow"), "Registered glow effect");
    check_true(host.has_effect("levels"), "Registered levels effect");
    check_true(host.has_effect("curves"), "Registered curves effect");
    check_true(host.has_effect("fill"), "Registered fill effect");
    check_true(host.has_effect("tint"), "Registered tint effect");
    check_true(host.has_effect("hue_saturation"), "Registered hue_saturation effect");
    check_true(host.has_effect("particle_emitter"), "Registered particle_emitter effect");

    RenderContext context;
    context.effects = create_effect_host();
    EffectHost::register_builtins(*context.effects);
    
    check_true(context.effects->has_effect("gaussian_blur"), "RenderContext auto-registers gaussian_blur");
    check_true(context.effects->has_effect("hue_saturation"), "RenderContext auto-registers hue_saturation");

    EffectParams blur_params;
    blur_params.scalars["blur_radius"] = 1.5f;
    const SurfaceRGBA blurred = host.apply("gaussian_blur", source, blur_params);
    check_true(blurred.get_pixel(8, 8).a > 0, "Blur keeps center visible");
    check_true(blurred.get_pixel(9, 8).a > 0, "Blur spreads to neighboring pixel");

    EffectParams hue_params;
    hue_params.scalars["hue"] = 180.0f;
    const SurfaceRGBA hued = host.apply("hue_saturation", source, hue_params);
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
    const SurfaceRGBA particles_a = host.apply("particle_emitter", source, particle_params);
    const SurfaceRGBA particles_b = host.apply("particle_emitter", source, particle_params);
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
    const SurfaceRGBA filled = host.apply("fill", source, fill_params);
    check_true(filled.get_pixel(8, 8).a >= 0.5f, "Fill correctly applies alpha");
    const Color p = filled.get_pixel(8, 8);
    check_true(std::abs(p.r - 200.0f/255.0f) < 0.001f && std::abs(p.g - 100.0f/255.0f) < 0.001f && std::abs(p.b - 50.0f/255.0f) < 0.001f, "Fill preserves straight RGB");

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

    return g_failures == 0;
}
