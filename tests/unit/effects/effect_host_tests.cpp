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

    check_true(host.has_effect("blur"), "Registered blur effect");
    check_true(host.has_effect("shadow"), "Registered shadow effect");

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

    return g_failures == 0;
}
