#pragma once

#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/renderer2d/effects/effect_descriptor.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tachyon {
namespace renderer2d {

/**
 * @brief Central registry for all rendering effects.
 * 
 * This is the single runtime registry for effects.
 */
class EffectRegistry {
public:
    EffectRegistry();
    ~EffectRegistry() = default;

    void register_spec(EffectDescriptor descriptor);
    const EffectDescriptor* find(std::string_view id) const;

    std::vector<std::string> list_ids() const;
    
    // Non-copyable
    EffectRegistry(const EffectRegistry&) = delete;
    EffectRegistry& operator=(const EffectRegistry&) = delete;

    // Movable
    EffectRegistry(EffectRegistry&&) noexcept = default;
    EffectRegistry& operator=(EffectRegistry&&) noexcept = default;
private:
    registry::TypedRegistry<EffectDescriptor> registry_;
};
} // namespace renderer2d
} // namespace tachyon

namespace tachyon { class TransitionRegistry; }

namespace tachyon {
namespace renderer2d {

void register_builtin_effects(EffectRegistry& registry, const TransitionRegistry& transition_registry);

} // namespace renderer2d
} // namespace tachyon
