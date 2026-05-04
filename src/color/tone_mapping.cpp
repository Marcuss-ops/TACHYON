#include "tachyon/color/tone_mapping.h"
#include "tachyon/color/aces.h"
#include <cmath>

namespace tachyon::color {

LinearRGBA tone_map(LinearRGBA hdr, ToneMappingOperator op) {
    switch (op) {
        case ToneMappingOperator::ACESFilmic:
            return aces_filmic(hdr);
        case ToneMappingOperator::Reinhard: {
            auto reinhard = [](float x) { return x / (1.0f + x); };
            return LinearRGBA{
                reinhard(hdr.r),
                reinhard(hdr.g),
                reinhard(hdr.b),
                hdr.a
            };
        }
        case ToneMappingOperator::Uncharted2: {
            // Simplified Uncharted 2 tone mapping
            const float A = 0.15f, B = 0.50f, C = 0.10f, D = 0.20f, E = 0.02f, F = 0.30f;
            auto uncharted = [=](float x) {
                return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F)) - E/F;
            };
            return LinearRGBA{
                uncharted(hdr.r),
                uncharted(hdr.g),
                uncharted(hdr.b),
                hdr.a
            };
        }
        default:
            return hdr;
    }
}

LinearRGBA adjust_exposure(LinearRGBA color, float exposure_stops) {
    float factor = std::pow(2.0f, exposure_stops);
    return LinearRGBA{
        color.r * factor,
        color.g * factor,
        color.b * factor,
        color.a
    };
}

} // namespace tachyon::color
