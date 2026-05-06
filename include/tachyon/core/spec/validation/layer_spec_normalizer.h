#pragma once

#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"

#include <string_view>
#include <vector>

namespace tachyon {
struct SceneSpec;
struct CompositionSpec;
}

namespace tachyon::core {

struct NormalizedLayerView {
    const ::tachyon::LayerSpec* source{nullptr};
    ::tachyon::LayerType type{::tachyon::LayerType::Unknown};
    std::string_view asset_reference;
    std::string_view preset_reference;
    std::string_view precomp_reference;
    bool legacy_type_string_used{false};
};

struct NormalizedCompositionView {
    const ::tachyon::CompositionSpec* source{nullptr};
    std::vector<NormalizedLayerView> layers;
};

struct NormalizedSceneView {
    const ::tachyon::SceneSpec* source{nullptr};
    std::vector<NormalizedCompositionView> compositions;
};

[[nodiscard]] NormalizedLayerView normalize_layer_view(const ::tachyon::LayerSpec& layer);
[[nodiscard]] ::tachyon::LayerType canonical_layer_type(const ::tachyon::LayerSpec& layer);
[[nodiscard]] NormalizedCompositionView normalize_composition_view(const ::tachyon::CompositionSpec& comp);
[[nodiscard]] NormalizedSceneView normalize_scene_view(const ::tachyon::SceneSpec& scene);

} // namespace tachyon::core
