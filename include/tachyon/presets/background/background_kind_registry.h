#pragma once

#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/core/spec/schema/objects/procedural_spec.h"
#include "tachyon/presets/background/background_params.h"
#include "tachyon/core/registry/parameter_schema.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include "tachyon/presets/background/background_specs.h"
#include "tachyon/presets/background/background_manifest.h"

namespace tachyon::presets {

class BackgroundKindRegistry : public registry::TypedRegistry<BackgroundKindSpec> {
public:
    explicit BackgroundKindRegistry(const BackgroundManifest& manifest) {
        load_from_manifest(manifest);
    }
    ~BackgroundKindRegistry() = default;

    [[nodiscard]] std::optional<ProceduralSpec> create(
        std::string_view id,
        const registry::ParameterBag& params
    ) const;

private:
    void load_from_manifest(const BackgroundManifest& manifest);
};

} // namespace tachyon::presets
