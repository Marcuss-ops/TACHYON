#include "tachyon/renderer3d/modifiers/modifier3d_registry.h"
#include "tachyon/renderer3d/modifiers/parallax_3d_modifier.h"
#include "tachyon/renderer3d/modifiers/tilt_3d_modifier.h"

#include <utility>
#include <vector>

namespace tachyon::renderer3d {

namespace {

std::vector<Modifier3DDescriptor> get_builtin_modifier3d_descriptors() {
    std::vector<Modifier3DDescriptor> descriptors;

    descriptors.push_back({
        "tachyon.modifier3d.tilt",
        {"tachyon.modifier3d.tilt", "Tilt", "Rotate the mesh around the X/Y axes.", "modifier.3d", {"tilt", "rotation"}},
        registry::ParameterSchema({
            {"amount_x", "Amount X", "Tilt amount around X axis", 0.0},
            {"amount_y", "Amount Y", "Tilt amount around Y axis", 0.0},
            {"smoothness", "Smoothness", "Animation smoothness", 0.5, 0.0, 1.0}
        }),
        []() {
            return std::make_unique<Tilt3DModifier>();
        }
    });

    descriptors.push_back({
        "tachyon.modifier3d.parallax",
        {"tachyon.modifier3d.parallax", "Parallax", "Offset the mesh by depth.", "modifier.3d", {"parallax", "depth"}},
        registry::ParameterSchema({
            {"factor", "Factor", "Parallax intensity", 1.0, 0.0, 10.0},
            {"auto_depth", "Auto Depth", "Automatically calculate depth", true}
        }),
        []() {
            return std::make_unique<Parallax3DModifier>();
        }
    });

    return descriptors;
}

} // namespace

Modifier3DRegistry::Modifier3DRegistry() {
    auto descriptors = get_builtin_modifier3d_descriptors();
    for (auto& desc : descriptors) {
        register_spec(std::move(desc));
    }
}

void Modifier3DRegistry::register_spec(Modifier3DDescriptor descriptor) {
    if (descriptor.id.empty()) {
        return;
    }
    registry_.register_spec(std::move(descriptor));
}

const Modifier3DDescriptor* Modifier3DRegistry::find(std::string_view id) const {
    return registry_.find(id);
}

std::unique_ptr<I3DModifier> Modifier3DRegistry::create(const std::string& id) const {
    if (const auto* descriptor = find(id)) {
        return descriptor->factory();
    }
    return nullptr;
}

std::vector<std::string> Modifier3DRegistry::list_ids() const {
    return registry_.list_ids();
}

} // namespace tachyon::renderer3d
