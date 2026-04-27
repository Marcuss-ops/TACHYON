#include "tachyon/renderer2d/color/blending.h"
#include <string>
#include <unordered_map>

namespace tachyon::renderer2d {

BlendMode parse_blend_mode(const std::string& name) {
    static const std::unordered_map<std::string, BlendMode> mode_map = {
        {"normal", BlendMode::Normal},
        {"multiply", BlendMode::Multiply},
        {"screen", BlendMode::Screen},
        {"overlay", BlendMode::Overlay},
        {"darken", BlendMode::Darken},
        {"lighten", BlendMode::Lighten},
        {"color_dodge", BlendMode::ColorDodge},
        {"color_burn", BlendMode::ColorBurn},
        {"hard_light", BlendMode::HardLight},
        {"soft_light", BlendMode::SoftLight},
        {"difference", BlendMode::Difference},
        {"exclusion", BlendMode::Exclusion},
        {"hue", BlendMode::Hue},
        {"saturation", BlendMode::Saturation},
        {"color", BlendMode::Color},
        {"luminosity", BlendMode::Luminosity},
        {"additive", BlendMode::Additive},
        {"linear_dodge", BlendMode::LinearDodge},
        {"subtract", BlendMode::Subtract},
        {"divide", BlendMode::Divide},
        {"linear_burn", BlendMode::LinearBurn},
        {"vivid_light", BlendMode::VividLight},
        {"linear_light", BlendMode::LinearLight},
        {"pin_light", BlendMode::PinLight},
        {"hard_mix", BlendMode::HardMix},
        {"darker_color", BlendMode::DarkerColor},
        {"lighter_color", BlendMode::LighterColor},
        {"stencil_alpha", BlendMode::StencilAlpha},
        {"stencil_luma", BlendMode::StencilLuma},
        {"silhouette_alpha", BlendMode::SilhouetteAlpha},
        {"silhouette_luma", BlendMode::SilhouetteLuma},
        {"alpha_add", BlendMode::AlphaAdd},
        {"luminescent_premult", BlendMode::LuminescentPremult}
    };

    auto it = mode_map.find(name);
    return it != mode_map.end() ? it->second : BlendMode::Normal;
}

} // namespace tachyon::renderer2d
