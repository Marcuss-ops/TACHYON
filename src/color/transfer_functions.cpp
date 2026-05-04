#include "tachyon/color/transfer_functions.h"
#include "tachyon/color/color_space.h"
#include <cmath>

namespace tachyon::color {

SRGBA apply_transfer(LinearRGBA linear, TransferFunction func) {
    switch (func) {
        case TransferFunction::sRGB: return linear_to_srgb(linear);
        case TransferFunction::Linear: return SRGBA{linear.r, linear.g, linear.b, linear.a};
        case TransferFunction::Gamma22: {
            float igamma = 1.0f / 2.2f;
            return SRGBA{
                std::pow(linear.r, igamma),
                std::pow(linear.g, igamma),
                std::pow(linear.b, igamma),
                linear.a
            };
        }
        default: return linear_to_srgb(linear);
    }
}

LinearRGBA remove_transfer(LinearRGBA non_linear, TransferFunction func) {
    switch (func) {
        case TransferFunction::sRGB: return srgb_to_linear(SRGBA{non_linear.r, non_linear.g, non_linear.b, non_linear.a});
        case TransferFunction::Linear: return non_linear;
        case TransferFunction::Gamma22: {
            return LinearRGBA{
                std::pow(non_linear.r, 2.2f),
                std::pow(non_linear.g, 2.2f),
                std::pow(non_linear.b, 2.2f),
                non_linear.a
            };
        }
        default: return non_linear;
    }
}

} // namespace tachyon::color
