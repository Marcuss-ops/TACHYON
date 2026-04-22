#pragma once

#include "tachyon/renderer2d/color/color_profile.h"
#include "tachyon/renderer2d/color/color_transform_graph.h"
#include "tachyon/renderer2d/color/color_management.h"
#include <string>
#include <memory>

namespace tachyon::renderer2d {

// ---------------------------------------------------------------------------
// ColorManagementSystem
//
// Central colour pipeline with four named slots:
//
//   input_profile   – source footage colour space (e.g. camera log, sRGB)
//   working_profile – internal compositing space  (default: ACEScg linear)
//   output_profile  – delivery space              (e.g. Rec.709, sRGB)
//   view_profile    – on-screen monitoring        (e.g. sRGB display)
//
// Usage:
//   auto cms = ColorManagementSystem::default_pipeline();
//   ColorTransformGraph g = cms.build_input_to_working();
//   g.process_surface(surface, cms.working_profile);
//
// OCIO (optional, compile with TACHYON_OCIO):
//   cms.load_ocio_config("/path/to/config.ocio");
//   // After load, build_input_to_working() returns an OCIO-driven graph.
// ---------------------------------------------------------------------------
class ColorManagementSystem {
public:
    ColorManagementSystem();

    ColorProfile input_profile  {ColorProfile::sRGB()};
    ColorProfile working_profile{ColorProfile::ACEScg()};
    ColorProfile output_profile {ColorProfile::Rec709()};
    ColorProfile view_profile   {ColorProfile::sRGB()};

    // Build a transform graph from input footage → working space.
    ColorTransformGraph build_input_to_working() const;

    // Build a transform graph from working space → delivery output.
    ColorTransformGraph build_working_to_output() const;

    // Build a transform graph from working space → display (monitoring).
    ColorTransformGraph build_working_to_view() const;

    // Optional OCIO integration.
    bool load_ocio_config(const std::string& config_path);

    bool uses_ocio() const;

    static ColorManagementSystem default_pipeline();
    static ColorManagementSystem srgb_passthrough();

private:
    std::shared_ptr<IColorManager> m_manager;
};

} // namespace tachyon::renderer2d
