#include "tachyon/renderer2d/effect_host.h"

#include <iostream>
#include <memory>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_effect_host_tests() {
    using namespace tachyon::renderer2d;

    SurfaceRGBA source(16, 16);
    source.clear(Color::transparent());
    source.set_pixel(8, 8, Color::white());

    EffectHost host;
    host.register_effect("blur", std::make_unique<GaussianBlurEffect>());
    host.register_effect("shadow", std::make_unique<DropShadowEffect>());
    host.register_effect("glow", std::make_unique<GlowEffect>());
    host.register_effect("levels", std::make_unique<LevelsEffect>());
    host.register_effect("curves", std::make_unique<CurvesEffect>());

    check_true(host.has_effect("blur"), "Registered blur effect");
    check_true(host.has_effect("shadow"), "Registered shadow effect");
    check_true(host.has_effect("glow"), "Registered glow effect");
    check_true(host.has_effect("levels"), "Registered levels effect");
    check_true(host.has_effect("curves"), "Registered curves effect");

    EffectParams blur_params;
    blur_params.scalars["blur_radius"] = 1.5f;
    const SurfaceRGBA blurred = host.apply("blur", source, blur_params);
    check_true(blurred.get_pixel(8, 8).a > 0, "Blur keeps center visible");
    check_true(blurred.get_pixel(9, 8).a > 0, "Blur spreads to neighboring pixel");

    EffectParams shadow_params;
    shadow_params.scalars["blur_radius"] = 1.5f;
    shadow_params.scalars["offset_x"] = 2.0f;
    shadow_params.scalars["offset_y"] = 2.0f;
    shadow_params.colors["shadow_color"] = Color{0, 0, 0, 200};
    const SurfaceRGBA shadowed = host.apply("shadow", source, shadow_params);
    check_true(shadowed.get_pixel(8, 8).a > 0, "Drop shadow preserves source pixel");
    check_true(shadowed.get_pixel(10, 10).a > 0, "Drop shadow offsets blurred alpha");

    EffectParams glow_params;
    glow_params.scalars["radius"] = 1.5f;
    glow_params.scalars["strength"] = 1.0f;
    const SurfaceRGBA glowed = host.apply("glow", source, glow_params);
    check_true(glowed.get_pixel(8, 8).a == source.get_pixel(8, 8).a, "Glow preserves alpha");
    check_true(glowed.get_pixel(8, 8).r >= source.get_pixel(8, 8).r, "Glow keeps center visible");
    check_true(glowed.get_pixel(9, 8).a == 0, "Glow does not spread alpha to empty background");
    check_true(glowed.get_pixel(9, 8).r > 0, "Glow spreads RGB to neighboring pixel");

    SurfaceRGBA levels_source(1, 1);
    levels_source.set_pixel(0, 0, Color{64, 64, 64, 255});
    EffectParams levels_params;
    levels_params.scalars["input_black"] = 0.0f;
    levels_params.scalars["input_white"] = 0.5f;
    levels_params.scalars["gamma"] = 1.0f;
    levels_params.scalars["output_black"] = 0.0f;
    levels_params.scalars["output_white"] = 255.0f;
    const SurfaceRGBA leveled = host.apply("levels", levels_source, levels_params);
    check_true(leveled.get_pixel(0, 0).r >= 127 && leveled.get_pixel(0, 0).r <= 129, "Levels remaps midtones");
    check_true(leveled.get_pixel(0, 0).a == 255, "Levels preserves alpha");

    SurfaceRGBA curves_source(1, 1);
    curves_source.set_pixel(0, 0, Color{64, 64, 64, 255});
    EffectParams curves_params;
    curves_params.scalars["amount"] = 1.0f;
    const SurfaceRGBA curved = host.apply("curves", curves_source, curves_params);
    check_true(curved.get_pixel(0, 0).r < curves_source.get_pixel(0, 0).r, "Curves darkens shadows");
    check_true(curved.get_pixel(0, 0).a == 255, "Curves preserves alpha");

    return g_failures == 0;
}
