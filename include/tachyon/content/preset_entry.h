#pragma once

#include "tachyon/content/content_kind.h"
#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/registry/registry_metadata.h"
#include <any>
#include <functional>
#include <string>
#include <vector>

namespace tachyon::content {

/**
 * @brief Unified factory type for all presets.
 * Returns a spec object (e.g., LayerSpec, TextAnimatorSpec) wrapped in std::any.
 */
using PresetFactory = std::function<std::any(const registry::ParameterBag&)>;

/**
 * @brief Entry in the ContentCatalog.
 */
struct PresetEntry {
    std::string id;
    ContentKind kind;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
    PresetFactory factory;

    // Optional: Reference to the original domain-specific struct if needed
    // For now, metadata covers most needs.
};

} // namespace tachyon::content
