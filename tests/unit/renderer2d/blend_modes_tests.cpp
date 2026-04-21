#include "tachyon/renderer2d/rasterizer_ops.h"

#include <iostream>

namespace {

int g_failures = 0;

void check_true(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "[FAIL] " << message << '\n';
        ++g_failures;
    }
}

} // namespace

bool run_blend_modes_tests() {
    {
        const tachyon::renderer2d::Color src{1.0f, 1.0f, 1.0f, 1.0f};
        const tachyon::renderer2d::Color dst{64.0f / 255.0f, 128.0f / 255.0f, 192.0f / 255.0f, 1.0f};
        const auto pixel = tachyon::renderer2d::blend_mode_color(src, dst, tachyon::renderer2d::BlendMode::Overlay);
        check_true(pixel.r >= dst.r, "overlay blend should not darken the red channel for a white source");
        check_true(pixel.g >= dst.g, "overlay blend should not darken the green channel for a white source");
        check_true(pixel.b >= dst.b, "overlay blend should not darken the blue channel for a white source");
        check_true(pixel.a == 1.0f, "overlay blend should preserve opaque alpha");
    }

    {
        const tachyon::renderer2d::Color src{1.0f, 1.0f, 1.0f, 1.0f};
        const tachyon::renderer2d::Color dst{64.0f / 255.0f, 128.0f / 255.0f, 192.0f / 255.0f, 1.0f};
        const auto pixel = tachyon::renderer2d::blend_mode_color(src, dst, tachyon::renderer2d::BlendMode::SoftLight);
        check_true(pixel.r >= dst.r, "soft light should not darken with a white source");
        check_true(pixel.g >= dst.g, "soft light should not darken with a white source");
        check_true(pixel.b >= dst.b, "soft light should not darken with a white source");
        check_true(pixel.a == 1.0f, "soft light should preserve opaque alpha");
    }

    return g_failures == 0;
}
