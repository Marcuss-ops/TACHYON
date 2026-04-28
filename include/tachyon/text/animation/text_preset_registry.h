#pragma once

#include "tachyon/core/spec/schema/animation/text_animator_spec.h"

#include <string>
#include <functional>
#include <memory>

namespace tachyon::text {

/**
 * TextPresetSpec: specification for a text animator preset.
 * Contains metadata and a factory function to create the preset.
 */
struct TextPresetSpec {
    using FactoryFn = std::function<TextAnimatorSpec(const std::string& based_on,
                                                    double stagger_delay,
                                                    double reveal_duration)>;

    std::string id;
    std::string name;
    std::string description;
    FactoryFn factory;
};

/**
 * Centralized runtime registry for text animator presets.
 * Follows singleton pattern similar to TransitionRegistry.
 * Provides a single entry point for all text preset creations.
 */
class TextPresetRegistry {
public:
    static TextPresetRegistry& instance();

    void register_preset(const TextPresetSpec& spec);
    void unregister_preset(const std::string& id);

    const TextPresetSpec* find(const std::string& id) const;
    std::size_t count() const;
    const TextPresetSpec* get_by_index(std::size_t index) const;

    // Create a preset by ID with common parameters
    TextAnimatorSpec create(const std::string& id,
                           const std::string& based_on = "characters_excluding_spaces",
                           double stagger_delay = 0.03,
                           double reveal_duration = 0.5) const;

private:
    TextPresetRegistry();
    ~TextPresetRegistry();

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace tachyon::text
