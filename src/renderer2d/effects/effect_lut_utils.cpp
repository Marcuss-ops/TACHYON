#include "tachyon/renderer2d/effects/effect_common.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include <algorithm>

namespace tachyon::renderer2d {

std::array<float, 256> build_channel_lut(std::function<float(float)> mapper) {
    std::array<float, 256> lut{};
    for (std::size_t i = 0; i < 256; ++i) {
        const float in = static_cast<float>(i) / 255.0f;
        lut[i] = std::clamp(mapper(in), 0.0f, 1.0f);
    }
    return lut;
}

SurfaceRGBA apply_channel_lut(const SurfaceRGBA& input, const std::array<float, 256>& lut) {
    return transform_surface(input, [&](Color px) {
        if (px.a == 0) return Color::transparent();
        const auto lookup = [&](float channel) -> float {
            const std::size_t index = static_cast<std::size_t>(std::lround(std::clamp(channel, 0.0f, 1.0f) * 255.0f));
            return lut[index];
        };
        return Color{lookup(px.r), lookup(px.g), lookup(px.b), px.a};
    });
}

} // namespace tachyon::renderer2d
