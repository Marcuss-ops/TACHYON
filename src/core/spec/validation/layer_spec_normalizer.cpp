#include "tachyon/core/spec/validation/layer_spec_normalizer.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"

namespace tachyon::core {

LayerType canonical_layer_type(const ::tachyon::LayerSpec& layer) {
    if (layer.type != LayerType::Unknown) {
        return layer.type;
    }
    if (layer.type_string.empty()) {
        return LayerType::Unknown;
    }
    return layer_type_from_string(layer.type_string);
}

NormalizedLayerView normalize_layer_view(const ::tachyon::LayerSpec& layer) {
    NormalizedLayerView view;
    view.source = &layer;
    view.type = canonical_layer_type(layer);
    view.asset_reference = layer.asset_id;
    view.preset_reference = layer.preset_id;
    view.precomp_reference = layer.precomp_id.has_value() ? std::string_view(*layer.precomp_id) : std::string_view{};
    view.legacy_type_string_used = layer.type == LayerType::Unknown && !layer.type_string.empty();
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
