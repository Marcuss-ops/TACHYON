#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/core/transition/transition_effect_resolver.h"
#include "tachyon/renderer2d/color/color_profile.h"

namespace tachyon::renderer2d {

/**
 * @brief Renders diagnostic surfaces for transition effects.
 * 
 * This class handles diagnostic rendering (like magenta fallback)
 * that was previously inside GlslTransitionEffect.
 */
class DiagnosticSurface {
public:
    /**
     * @brief Renders a magenta diagnostic surface for failed transitions.
     * 
     * @param diagnostics The diagnostics information about the failure.
     * @param width Width of the output surface.
     * @param height Height of the output surface.
     * @param profile Color profile to use.
     * @return SurfaceRGBA filled with magenta color (1.0, 0.0, 1.0, 1.0).
     */
    static SurfaceRGBA render_magenta_fallback(const TransitionDiagnostics& diagnostics,
                                                 std::uint32_t width,
                                                 std::uint32_t height,
                                                 const ColorProfile& profile = ColorProfile::sRGB());

    /**
     * @brief Renders a diagnostic surface based on the resolution result.
     * 
     * @param resolved The resolved transition effect (may be invalid).
     * @param width Width of the output surface.
     * @param height Height of the output surface.
     * @param profile Color profile to use.
     * @return Diagnostic surface if invalid, empty surface otherwise.
     */
    static SurfaceRGBA render(const ResolvedTransitionEffect& resolved,
                               std::uint32_t width,
                               std::uint32_t height,
                               const ColorProfile& profile = ColorProfile::sRGB());
};

} // namespace tachyon::renderer2d
