#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/presets/text/text_params.h"
#include <string>
#include <string_view>
#include <functional>
#include <memory>
#include <vector>
#include <optional>

namespace tachyon::presets {

/**
 * @brief Specification for a text preset (style + layout).
 */
struct TextPresetSpec {
    std::string id;
    std::string name;
    std::string description;
    
    // Factory function to create a text layer.
    std::function<LayerSpec(const std::string& content, const TextParams& p)> factory;
};

/**
 * @brief Registry for text presets.
 */
class TextPresetRegistry {
public:
    static TextPresetRegistry& instance();

    void register_preset(TextPresetSpec spec);
    const TextPresetSpec* find(std::string_view id) const;
    
    std::optional<LayerSpec> create(std::string_view id, const std::string& content, const TextParams& p) const;

    std::vector<std::string> list_ids() const;
    void load_builtins();

private:
    TextPresetRegistry();
    ~TextPresetRegistry() = default;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace tachyon::presets
