#pragma once

#include "tachyon/content/content_kind.h"
#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/core/registry/registry_metadata.h"
#include <string>

namespace tachyon::content {

/**
 * @brief Entry in the ContentCatalog containing only metadata and schemas.
 */
struct PresetEntry {
    std::string id;
    ContentKind kind;
    registry::RegistryMetadata metadata;
    registry::ParameterSchema schema;
};

} // namespace tachyon::content
