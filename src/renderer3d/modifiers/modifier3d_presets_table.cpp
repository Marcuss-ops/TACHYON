#include "tachyon/renderer3d/modifiers/modifier3d_builtin_manifest.h"
#include "tachyon/renderer3d/modifiers/parallax_3d_modifier.h"
#include "tachyon/renderer3d/modifiers/tilt_3d_modifier.h"

#include <array>

namespace tachyon::renderer3d {

void Modifier3DRegistry::load_builtins() {
    static const std::array<Modifier3DBuiltinSpec, 2> kBuiltins = {{
        {
            "tachyon.modifier3d.tilt",
            "Tilt",
            "Rotate the mesh around the X/Y axes.",
            "modifier.3d",
            {"tilt", "rotation"},
            registry::ParameterSchema({
                {"amount_x", "Amount X", "Tilt amount around X axis", 0.0},
                {"amount_y", "Amount Y", "Tilt amount around Y axis", 0.0},
                {"smoothness", "Smoothness", "Animation smoothness", 0.5, 0.0, 1.0}
            }),
            make_modifier_factory<Tilt3DModifier>()
        },
        {
            "tachyon.modifier3d.parallax",
            "Parallax",
            "Offset the mesh by depth.",
            "modifier.3d",
            {"parallax", "depth"},
            registry::ParameterSchema({
                {"factor", "Factor", "Parallax intensity", 1.0, 0.0, 10.0},
                {"auto_depth", "Auto Depth", "Automatically calculate depth", true}
            }),
            make_modifier_factory<Parallax3DModifier>()
        }
    }};

    for (const auto& builtin : kBuiltins) {
        register_modifier_builtin(*this, builtin);
    }
}

} // namespace tachyon::renderer3d
