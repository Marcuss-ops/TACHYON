#pragma once

#include "tachyon/background_descriptor.h"
#include <string_view>
#include <vector>

namespace tachyon::presets {

/**
 * @brief Returns the canonical builtin background descriptors.
 *
 * This is the source of truth for builtin background metadata and builders.
 * Registries can copy from it, but runtime code should not rely on registry
 * singletons to resolve builtin background kinds.
 */
const std::vector<BackgroundDescriptor>& builtin_background_descriptors();

/**
 * @brief Finds a builtin background descriptor by canonical id.
 */
const BackgroundDescriptor* find_builtin_background_descriptor(std::string_view id);

} // namespace tachyon::presets
