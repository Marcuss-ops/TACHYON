#pragma once

#include "tachyon/renderer2d/effects/effect_descriptor.h"
#include "tachyon/core/spec/schema/effects/effect_spec.h"
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
    EvaluatedEffect spec;
    bool valid{false};
    std::string error_message;
};

/**
 * @brief Resolves an EvaluatedEffect into a ready-to-use effect descriptor.
 */
ResolvedEffect resolve_effect(const EvaluatedEffect& spec, const EffectRegistry& registry);

/**
 * @brief Resolves an effect by id string (convenience overload).
 * 
 * Prefer resolve_effect(const EffectSpec&, const EffectRegistry&) when you have the full spec.
 */
ResolvedEffect resolve_effect_by_id(const std::string& id, const EffectRegistry& registry);

} // namespace tachyon::renderer2d
