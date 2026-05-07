#pragma once

#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/renderer2d/effects/effect_descriptor.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tachyon::renderer2d {

/**
 * @brief Central registry for all rendering effects.
 */
class EffectRegistry {
public:
    EffectRegistry();
    ~EffectRegistry() = default;

    void register_spec(EffectDescriptor descriptor);
    const EffectDescriptor* find(std::string_view id) const;

    std::vector<std::string> list_ids() const;

    void register_builtins();

private:
    registry::TypedRegistry<EffectDescriptor> registry_;
};

} // namespace tachyon::renderer2d
