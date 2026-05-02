#include "tachyon/renderer3d/modifiers/three_d_modifier_registry.h"
#include "tachyon/renderer3d/modifiers/tilt_3d_modifier.h"
#include "tachyon/renderer3d/modifiers/parallax_3d_modifier.h"
#include <stdexcept>

namespace tachyon {
namespace renderer3d {

ThreeDModifierRegistry& ThreeDModifierRegistry::instance() {
    static ThreeDModifierRegistry registry;
    static bool initialized = false;
    if (!initialized) {
        registry.registerModifier("tilt", [](const ThreeDModifierSpec& spec) {
            return std::make_unique<Tilt3DModifier>(spec);
        });
        registry.registerModifier("parallax", [](const ThreeDModifierSpec& spec) {
            return std::make_unique<Parallax3DModifier>(spec);
        });
        initialized = true;
    }
    return registry;
}

void ThreeDModifierRegistry::registerModifier(const std::string& type, Factory factory) {
    factories_[type] = std::move(factory);
}

std::unique_ptr<I3DModifier> ThreeDModifierRegistry::create(const ThreeDModifierSpec& spec) const {
    auto it = factories_.find(spec.type);
    if (it == factories_.end()) {
        return nullptr;
    }
    return it->second(spec);
}

} // namespace renderer3d
} // namespace tachyon
