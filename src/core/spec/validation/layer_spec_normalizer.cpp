#include "tachyon/core/spec/validation/layer_spec_normalizer.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"

namespace tachyon::core {

LayerType canonical_layer_type(const ::tachyon::LayerSpec& layer) {
    if (layer.identity.type != LayerType::Unknown && layer.identity.type != LayerType::NullLayer) {
        return layer.identity.type;
    }
    
    return LayerType::Unknown;
}

NormalizedLayerView normalize_layer_view(const ::tachyon::LayerSpec& layer) {
    NormalizedLayerView view;
    view.source = &layer;
    view.type = layer.identity.type;
    std::visit([&](auto&& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, MediaSource>) {
            view.asset_reference = s.asset_path;
        } else if constexpr (std::is_same_v<T, ProceduralSource>) {
            view.preset_reference = s.kind;
        } else if constexpr (std::is_same_v<T, PrecompSource>) {
            view.precomp_reference = s.precomp_id;
        }
    }, layer.source);
    
    return view;
}

NormalizedCompositionView normalize_composition_view(const ::tachyon::CompositionSpec& comp) {
    NormalizedCompositionView view;
    view.source = &comp;
    view.layers.reserve(comp.layers.size());
    for (const auto& layer : comp.layers) {
        view.layers.push_back(normalize_layer_view(layer));
    }
    return view;
}

NormalizedSceneView normalize_scene_view(const ::tachyon::SceneSpec& scene) {
    NormalizedSceneView view;
    view.source = &scene;
    view.compositions.reserve(scene.compositions.size());
    for (const auto& comp : scene.compositions) {
        view.compositions.push_back(normalize_composition_view(comp));
    }
    return view;
}

} // namespace tachyon::core
