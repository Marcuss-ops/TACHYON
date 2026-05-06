#pragma once

#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"

#include <string_view>

namespace tachyon::core {

struct NormalizedLayerView {
    const ::tachyon::LayerSpec* source{nullptr};
    ::tachyon::LayerType type{::tachyon::LayerType::Unknown};
    std::string_view asset_reference;
    std::string_view preset_reference;
    std::string_view precomp_reference;
    bool legacy_type_string_used{false};
};

[[nodiscard]] NormalizedLayerView normalize_layer_view(const ::tachyon::LayerSpec& layer);
[[nodiscard]] ::tachyon::LayerType canonical_layer_type(const ::tachyon::LayerSpec& layer);

} // namespace tachyon::core
