#include "tachyon/renderer2d/effect_host.h"
#include "tachyon/renderer2d/render_context.h"
#include "tachyon/renderer2d/rasterizer.h"
#include "tachyon/renderer2d/framebuffer.h"

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

    EffectParams fill_params;
    fill_params.colors["color"] = Color{200, 100, 50, 128};
    const SurfaceRGBA filled = host.apply("fill", source, fill_params);
    check_true(filled.get_pixel(8, 8).a == 128, "Fill correctly applies alpha");
    const Color p = filled.get_pixel(8, 8);
    check_true(p.r == 200 && p.g == 100 && p.b == 50, "Fill preserves straight RGB");

    PrecompCache cache;
    cache.max_bytes = 16; 

    RasterizedFrame2D frame_a;
    frame_a.surface = SurfaceRGBA(1, 1);
    frame_a.surface->set_pixel(0, 0, Color::red());

    RasterizedFrame2D frame_b;
    frame_b.surface = SurfaceRGBA(1, 1);
    frame_b.surface->set_pixel(0, 0, Color::green());

    RasterizedFrame2D frame_c;
    frame_c.surface = SurfaceRGBA(1, 1);
    frame_c.surface->set_pixel(0, 0, Color::blue());

    cache.store("a", frame_a);
    cache.store("b", frame_b);
    
    RasterizedFrame2D probe;
    check_true(cache.lookup("a", probe), "Cache returns first entry");
    
    cache.store("c", frame_c);
    check_true(cache.lookup("a", probe), "Entry A survives");
    check_true(cache.lookup("b", probe), "Entry B survives");
    
    cache.max_bytes = 10;
    cache.clear();
    cache.max_bytes = 10;
    cache.store("a", frame_a); // [a]
    cache.store("b", frame_b); // [b, a]
    check_true(cache.lookup("a", probe), "Lookup A"); // [a, b]
    cache.store("c", frame_c); // [c, a] -> evicts b
    
    check_true(cache.lookup("a", probe), "A survives as it was used recently");
    check_true(!cache.lookup("b", probe), "B is evicted as it was LRU");
    check_true(cache.lookup("c", probe), "C is present");

    return g_failures == 0;
}
