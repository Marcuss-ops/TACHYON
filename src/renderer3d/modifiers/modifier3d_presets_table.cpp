#include "tachyon/renderer3d/modifiers/modifier3d_registry.h"
#include "tachyon/renderer3d/modifiers/parallax_3d_modifier.h"
#include "tachyon/renderer3d/modifiers/tilt_3d_modifier.h"

#include <utility>

namespace tachyon::renderer3d {

void Modifier3DRegistry::load_builtins() {
    register_spec({
        "tachyon.modifier3d.tilt",
        {"tachyon.modifier3d.tilt", "Tilt", "Rotate the mesh around the X/Y axes.", "modifier.3d", {"tilt", "rotation"}},
        [](const ThreeDModifierSpec& spec) {
            return std::make_unique<Tilt3DModifier>(spec);
        }
    });

    register_spec({
        "tachyon.modifier3d.parallax",
        {"tachyon.modifier3d.parallax", "Parallax", "Offset the mesh by depth.", "modifier.3d", {"parallax", "depth"}},
        [](const ThreeDModifierSpec& spec) {
            return std::make_unique<Parallax3DModifier>(spec);
        }
    });
}

} // namespace tachyon::renderer3d
