#pragma once

#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <string>
#include <vector>
#include <functional>

namespace tachyon {

enum class BackgroundStatus {
    Stable,
    Experimental,
    Deprecated
};

/**
 * @brief Logical kind of background.
 */
enum class BackgroundKind {
    Solid,
    LinearGradient,
    RadialGradient,
    Image,
    Video,
    Procedural
};

/**
 * @brief Capabilities of a background (which backends are supported).
 */
struct BackgroundCapabilities {
    bool supports_gpu{false};
    bool supports_cpu{false};
};

/**
 * @brief Background build function signature.
 * Creates a LayerSpec from the provided parameters.
 */
using BackgroundBuildFn = std::function<LayerSpec(const registry::ParameterBag&)>;

/**
 * @brief Unified descriptor for a Tachyon background (SINGLE SOURCE OF TRUTH).
 */
struct BackgroundDescriptor {
    std::string id;
    std::vector<std::string> aliases;

    BackgroundKind kind;
    registry::ParameterSchema params;
    BackgroundCapabilities capabilities;

    BackgroundBuildFn build;

    std::string display_name;
    std::string description;
    BackgroundStatus status{BackgroundStatus::Stable};
};

} // namespace tachyon
