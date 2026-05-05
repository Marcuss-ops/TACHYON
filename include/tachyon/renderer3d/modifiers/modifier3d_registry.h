#pragma once

#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/renderer3d/modifiers/i3d_modifier.h"
#include "tachyon/core/spec/schema/3d/three_d_spec.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tachyon::renderer3d {

/**
 * @brief Descriptor for a 3D modifier.
 */
struct Modifier3DDescriptor {
    std::string id;
    registry::RegistryMetadata metadata;
    using Factory = std::function<std::unique_ptr<I3DModifier>(const ThreeDModifierSpec&)>;
    Factory factory;
};

/**
 * @brief Registry for 3D modifiers (e.g., bend, tilt, parallax).
 */
class Modifier3DRegistry {
public:
    static Modifier3DRegistry& instance();

    void register_spec(Modifier3DDescriptor descriptor);
    const Modifier3DDescriptor* find(std::string_view id) const;
    std::unique_ptr<I3DModifier> create(const ThreeDModifierSpec& spec) const;
    [[nodiscard]] std::vector<std::string> list_ids() const;

    void load_builtins();

private:
    Modifier3DRegistry();
    ~Modifier3DRegistry() = default;

    registry::TypedRegistry<Modifier3DDescriptor> registry_;
};

} // namespace tachyon::renderer3d
