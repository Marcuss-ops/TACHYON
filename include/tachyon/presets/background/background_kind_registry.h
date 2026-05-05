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
#include <vector>

namespace tachyon::presets {

namespace background::procedural_bg {
struct ProceduralParams;
}

struct BackgroundKindSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    std::function<ProceduralSpec(const registry::ParameterBag&)> factory;
};

class BackgroundKindRegistry {
public:
    static BackgroundKindRegistry& instance();

    void register_spec(BackgroundKindSpec spec);
    const BackgroundKindSpec* find(std::string_view id) const;

    [[nodiscard]] std::optional<ProceduralSpec> create(
        std::string_view id,
        const registry::ParameterBag& params
    ) const;

    [[nodiscard]] std::vector<std::string> list_ids() const;

    void load_builtins();

private:
    BackgroundKindRegistry();
    ~BackgroundKindRegistry() = default;

    registry::TypedRegistry<BackgroundKindSpec> registry_;
};

} // namespace tachyon::presets
