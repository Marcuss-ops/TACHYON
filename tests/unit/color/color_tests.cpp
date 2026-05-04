#include "tachyon/color/color_space.h"
#include "tachyon/color/premultiplied_alpha.h"
#include "tachyon/color/aces.h"
#include "tachyon/color/blending.h"
#include <cmath>
#include <cassert>

static bool approx_eq(float a, float b, float eps = 1e-5f) {
    return std::fabs(a - b) < eps;
}

int main() {
    using namespace tachyon::color;

    // Test sRGB -> linear -> sRGB roundtrip
    {
        SRGBA srgb{0.1f, 0.5f, 0.9f, 1.0f};
        LinearRGBA linear = srgb_to_linear(srgb);
        SRGBA back = linear_to_srgb(linear);
        assert(approx_eq(srgb.r, back.r));
        assert(approx_eq(srgb.g, back.g));
        assert(approx_eq(srgb.b, back.b));
    }

    // Test premultiply / unpremultiply
    {
        LinearRGBA c{0.5f, 0.6f, 0.7f, 0.5f};
        LinearRGBA pre = premultiply(c);
        assert(approx_eq(pre.r, 0.25f));
        assert(approx_eq(pre.g, 0.3f));
        assert(approx_eq(pre.b, 0.35f));
        
        LinearRGBA unpre = unpremultiply(pre);
        assert(approx_eq(unpre.r, c.r));
        assert(approx_eq(unpre.g, c.g));
        assert(approx_eq(unpre.b, c.b));
    }

    // Test unpremultiply with alpha=0
    {
        LinearRGBA c{0.5f, 0.6f, 0.7f, 0.0f};
        LinearRGBA unpre = unpremultiply(c);
        assert(approx_eq(unpre.r, 0.0f));
        assert(approx_eq(unpre.g, 0.0f));
        assert(approx_eq(unpre.b, 0.0f));
    }

    // Test ACES filmic produces stable output
    {
        LinearRGBA hdr{2.0f, 3.0f, 1.5f, 1.0f};
        LinearRGBA result = aces_filmic(hdr);
        assert(result.r >= 0.0f && result.r <= 1.0f);
        assert(result.g >= 0.0f && result.g <= 1.0f);
        assert(result.b >= 0.0f && result.b <= 1.0f);
        // Same input gives same output
        LinearRGBA result2 = aces_filmic(hdr);
        assert(approx_eq(result.r, result2.r));
    }

    // Test blend normal
    {
        LinearRGBA src{0.5f, 0.0f, 0.0f, 0.5f};
        LinearRGBA dst{0.0f, 0.5f, 0.0f, 1.0f};
        LinearRGBA blend = blend_normal(src, dst);
        float expected_a = 0.5f + 1.0f * (1.0f - 0.5f); // 1.0f
        assert(approx_eq(blend.a, expected_a));
    }

    return 0;
}
