#pragma once

#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include <string>
#include <string_view>
#include <functional>
#include <memory>
#include <vector>
#include <optional>

namespace tachyon::presets {

/**
 * @brief Specification for a text animator preset.
 */
struct TextAnimatorPresetSpec {
    using FactoryFn = std::function<std::vector<TextAnimatorSpec>(const std::string& based_on,
                                                                 double stagger_delay,
                                                                 double reveal_duration)>;

    std::string id;
    registry::RegistryMetadata metadata;
    FactoryFn factory;
};

/**
 * @brief Registry for text animator presets.
 */
class TextAnimatorPresetRegistry {
public:
    static TextAnimatorPresetRegistry& instance();

    void register_spec(TextAnimatorPresetSpec spec);
    const TextAnimatorPresetSpec* find(std::string_view id) const;

    std::vector<TextAnimatorSpec> create(std::string_view id,
                                         const std::string& based_on = "characters_excluding_spaces",
                                         double stagger_delay = 0.03,
                                         double reveal_duration = 0.5) const;

    std::vector<std::string> list_ids() const;
    void load_builtins();

private:
    TextAnimatorPresetRegistry();
    ~TextAnimatorPresetRegistry() = default;

    registry::TypedRegistry<TextAnimatorPresetSpec> registry_;
};

} // namespace tachyon::presets
