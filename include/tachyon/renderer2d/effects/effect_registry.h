#pragma once

#include "tachyon/renderer2d/effects/effect_descriptor.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

namespace tachyon::renderer2d {

/**
 * @brief Central registry for all rendering effects.
 */
class EffectRegistry {
public:
    static EffectRegistry& instance();

    /**
     * @brief Registers a new effect.
     */
    void register_effect(EffectDescriptor descriptor);

    /**
     * @brief Finds an effect by its ID.
     */
    const EffectDescriptor* find(const std::string& id) const;

    /**
     * @brief Returns all registered effects.
     */
    std::vector<const EffectDescriptor*> get_all() const;

    /**
     * @brief Helper to register built-in effects.
     */
    void register_builtins();

private:
    EffectRegistry() = default;
    std::unordered_map<std::string, EffectDescriptor> m_effects;
};

} // namespace tachyon::renderer2d
