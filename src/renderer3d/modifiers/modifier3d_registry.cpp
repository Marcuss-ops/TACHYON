#include "tachyon/renderer3d/modifiers/modifier3d_registry.h"

namespace tachyon::renderer3d {

std::vector<Modifier3DDescriptor> get_builtin_modifier3d_descriptors();

Modifier3DRegistry::Modifier3DRegistry() {
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

void register_builtin_modifiers(Modifier3DRegistry& registry) {
    auto descriptors = get_builtin_modifier3d_descriptors();
    for (auto& desc : descriptors) {
        registry.register_spec(std::move(desc));
    }
}

} // namespace tachyon::renderer3d
