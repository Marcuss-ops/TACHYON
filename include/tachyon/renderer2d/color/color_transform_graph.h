#pragma once

#include "tachyon/renderer2d/color/color_transform.h"
#include <vector>
#include <functional>

namespace tachyon::renderer2d {

class ColorTransformGraph {
public:
    using TransformStep = std::function<Color(Color)>;

    void add_step(TransformStep step) {
        m_steps.push_back(std::move(step));
    }

    void build_from_to(const ColorProfile& src, const ColorProfile& dst) {
        m_steps.clear();

        // 1. Source EOTF (Non-linear to Linear)
        if (src.curve != TransferCurve::Linear) {
            add_step([curve = src.curve](Color c) {
                return Color{
                    TransferFunctions::to_linear(c.r, curve),
                    TransferFunctions::to_linear(c.g, curve),
                    TransferFunctions::to_linear(c.b, curve),
                    c.a
                };
            });
        }

        // 2. Primaries Conversion (Linear space)
        if (src.primaries != dst.primaries || src.white_point != dst.white_point) {
            auto ps_src = get_primary_set(src.primaries, src.white_point);
            auto ps_dst = get_primary_set(dst.primaries, dst.white_point);
            
            auto m_src_to_xyz = calculate_rgb_to_xyz(ps_src);
            auto m_dst_to_xyz = calculate_rgb_to_xyz(ps_dst);
            auto m_xyz_to_dst = invert(m_dst_to_xyz);
            
            auto m_conv = multiply(m_xyz_to_dst, m_src_to_xyz);

            add_step([m = m_conv](Color c) {
                return Color{
                    c.r * m.m[0] + c.g * m.m[1] + c.b * m.m[2],
                    c.r * m.m[3] + c.g * m.m[4] + c.b * m.m[5],
                    c.r * m.m[6] + c.g * m.m[7] + c.b * m.m[8],
                    c.a
                };
            });
        }

        // 3. Destination OETF (Linear to Non-linear)
        if (dst.curve != TransferCurve::Linear) {
            add_step([curve = dst.curve](Color c) {
                return Color{
                    TransferFunctions::from_linear(c.r, curve),
                    TransferFunctions::from_linear(c.g, curve),
                    TransferFunctions::from_linear(c.b, curve),
                    c.a
                };
            });
        }
    }

    Color process(Color c) const {
        for (const auto& step : m_steps) {
            c = step(c);
        }
        return c;
    }

    void process_surface(SurfaceRGBA& surface, const ColorProfile& target_profile) {
        if (surface.profile() == target_profile) return;

        build_from_to(surface.profile(), target_profile);
        
        auto& pixels = surface.mutable_pixels();
        for (size_t i = 0; i < pixels.size(); i += 4) {
            Color c{pixels[i], pixels[i+1], pixels[i+2], pixels[i+3]};
            c = process(c);
            pixels[i] = c.r;
            pixels[i+1] = c.g;
            pixels[i+2] = c.b;
            pixels[i+3] = c.a;
        }

        surface.set_profile(target_profile);
    }

private:
    std::vector<TransformStep> m_steps;
};

} // namespace tachyon::renderer2d
