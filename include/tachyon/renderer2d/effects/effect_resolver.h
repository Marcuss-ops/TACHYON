#pragma once

#include "tachyon/renderer2d/effects/effect_descriptor.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include <string>

namespace tachyon::renderer2d {

class EffectRegistry;

/**
 * @brief Resolved effect information for renderers.
 * 
 * This is the output of the effect resolver. Renderers should use
 * this struct instead of resolving effect ids themselves.
 */
struct ResolvedEffect {
    std::string id;
    const EffectDescriptor* descriptor{nullptr};
    EffectSpec spec;
    bool valid{false};
    std::string error_message;
};

/**
 * @brief Resolves an EffectSpec into a ready-to-use effect.
 * 
 * This is the SINGLE PATH for effect resolution. Renderers must not
 * implement their own id lookup or switch-case logic.
 * 
 * @param spec The effect specification from the layer.
 * @param registry The effect registry to look up descriptors.
 * @return ResolvedEffect with all needed information.
 */
ResolvedEffect resolve_effect(const EffectSpec& spec, const EffectRegistry& registry);

/**
 * @brief Resolves an effect by id string (convenience overload).
 * 
 * Prefer resolve_effect(const EffectSpec&, const EffectRegistry&) when you have the full spec.
 */
ResolvedEffect resolve_effect_by_id(const std::string& id, const EffectRegistry& registry);

} // namespace tachyon::renderer2d
