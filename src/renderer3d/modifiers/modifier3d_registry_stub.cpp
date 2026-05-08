#include "tachyon/renderer3d/modifiers/modifier3d_registry.h"

namespace tachyon::renderer3d {

Modifier3DRegistry::Modifier3DRegistry() = default;

void Modifier3DRegistry::register_spec(Modifier3DDescriptor /*descriptor*/) {}

const Modifier3DDescriptor* Modifier3DRegistry::find(std::string_view /*id*/) const {
    return nullptr;
}

std::unique_ptr<I3DModifier> Modifier3DRegistry::create(const std::string& /*id*/) const {
    return nullptr;
}

std::vector<std::string> Modifier3DRegistry::list_ids() const {
    return {};
}

} // namespace tachyon::renderer3d
