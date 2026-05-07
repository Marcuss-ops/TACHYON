#include "tachyon/renderer2d/effects/diagnostics/diagnostic_surface.h"
#include <iostream>

namespace tachyon::renderer2d {

SurfaceRGBA DiagnosticSurface::render_magenta_fallback(const TransitionDiagnostics& diagnostics,
                                                        std::uint32_t width,
                                                        std::uint32_t height,
                                                        const ColorProfile& profile) {
    if (diagnostics.has_error) {
        std::cerr << "WARNING: Transition resolution failed for '" 
                  << diagnostics.transition_id << "': " 
                  << diagnostics.error_message << "\n";
    }
    
    SurfaceRGBA output(width, height);
    output.set_profile(profile);
    output.clear(Color{1.0f, 0.0f, 1.0f, 1.0f}); // Diagnostic Magenta
    return output;
}

SurfaceRGBA DiagnosticSurface::render(const ResolvedTransitionEffect& resolved,
                                      std::uint32_t width,
                                      std::uint32_t height,
                                      const ColorProfile& profile) {
    if (!resolved.valid) {
        return render_magenta_fallback(resolved.diagnostics, width, height, profile);
    }
    return SurfaceRGBA(width, height); // Return empty surface if valid
}

} // namespace tachyon::renderer2d
