#pragma once

#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/renderer2d/effects/effect_descriptor.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tachyon { class TransitionRegistry; }

namespace tachyon::renderer2d {

/**
 * @brief Central registry for all rendering effects.
 * 
 * This is the single runtime registry for effects.
 * Use EffectManifest to generate descriptors, then register them here.
 */
class EffectRegistry {
public:
    EffectRegistry();
    ~EffectRegistry() = default;

    void register_spec(EffectDescriptor descriptor);
    const EffectDescriptor* find(std::string_view id) const;

    std::vector<std::string> list_ids() const;
private:
    registry::TypedRegistry<EffectDescriptor> registry_;
};

/**
 * @brief Register builtin effects using the EffectManifest.
 * 
 * This is the consolidated bootstrap function. It should be called once
 * from the runtime initialization (RenderSession), not from multiple renderers.
 */
void register_builtin_effects(EffectRegistry& registry, const TransitionRegistry& transition_registry);

} // namespace tachyon::renderer2d
