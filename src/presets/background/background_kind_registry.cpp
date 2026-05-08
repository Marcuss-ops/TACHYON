#include "tachyon/presets/background/background_kind_registry.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/core/registry/parameter_bag.h"
#include <string>

namespace tachyon::presets {

std::optional<ProceduralSpec> BackgroundKindRegistry::create(
    std::string_view id,
    const registry::ParameterBag& params) const {
    if (const auto* spec = find(id)) {
        return spec->factory(params);
    }
    return std::nullopt;
}

void BackgroundKindRegistry::load_from_manifest(const BackgroundManifest& manifest) {
    auto specs = manifest.generate_kind_specs();
    for (auto& spec : specs) {
        register_spec(std::move(spec));
    }
}

} // namespace tachyon::presets
