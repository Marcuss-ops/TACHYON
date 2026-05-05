#include "tachyon/presets/background/background_kind_registry.h"
#include "tachyon/presets/background/background_builders.h"
#include "tachyon/presets/background/procedural.h"

namespace tachyon::presets {

BackgroundKindRegistry& BackgroundKindRegistry::instance() {
    static BackgroundKindRegistry registry;
    return registry;
}

BackgroundKindRegistry::BackgroundKindRegistry() {
    load_builtins();
}

void BackgroundKindRegistry::register_spec(BackgroundKindSpec spec) {
    registry_.register_spec(std::move(spec));
}

const BackgroundKindSpec* BackgroundKindRegistry::find(std::string_view id) const {
    return registry_.find(id);
}

std::optional<ProceduralSpec> BackgroundKindRegistry::create(
    std::string_view id,
    const registry::ParameterBag& params) const {
    if (const auto* spec = find(id)) {
        return spec->factory(params);
    }
    return std::nullopt;
}

std::vector<std::string> BackgroundKindRegistry::list_ids() const {
    return registry_.list_ids();
}

} // namespace tachyon::presets
