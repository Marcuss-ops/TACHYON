#include "tachyon/renderer3d/modifiers/modifier3d_registry.h"
#include "tachyon/renderer3d/modifiers/parallax_3d_modifier.h"
#include "tachyon/renderer3d/modifiers/tilt_3d_modifier.h"

#include <utility>

namespace tachyon::renderer3d {

void Modifier3DRegistry::load_builtins() {
    using namespace registry;

    register_spec({
        "tachyon.modifier3d.tilt",
        {"tachyon.modifier3d.tilt", "Tilt", "Rotate the mesh around the X/Y axes.", "modifier.3d", {"tilt", "rotation"}},
        ParameterSchema({
            {"amount_x", "Amount X", "Tilt amount around X axis", 0.0},
            {"amount_y", "Amount Y", "Tilt amount around Y axis", 0.0},
            {"smoothness", "Smoothness", "Animation smoothness", 0.5, 0.0, 1.0}
        }),
        [](const ThreeDModifierSpec& spec) {
            return std::make_unique<Tilt3DModifier>(spec);
        }
    });

    register_spec({
        "tachyon.modifier3d.parallax",
        {"tachyon.modifier3d.parallax", "Parallax", "Offset the mesh by depth.", "modifier.3d", {"parallax", "depth"}},
        ParameterSchema({
            {"factor", "Factor", "Parallax intensity", 1.0, 0.0, 10.0},
            {"auto_depth", "Auto Depth", "Automatically calculate depth", true}
        }),
        [](const ThreeDModifierSpec& spec) {
            return std::make_unique<Parallax3DModifier>(spec);
        }
    });
}

} // namespace tachyon::renderer3d
