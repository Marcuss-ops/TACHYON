#include "tachyon/core/spec/validation/scene_normalizer.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"

namespace tachyon::core {

void SceneNormalizer::normalize(SceneSpec& scene) {
    for (auto& comp : scene.compositions) {
        bool has_background_layer = std::any_of(comp.layers.begin(), comp.layers.end(),
            [](const auto& l) { return l.identity.id == "__background__"; });

        if (comp.background.has_value() && !has_background_layer) {
            LayerSpec bg_layer;
            bg_layer.identity.id = "__background__";
            bg_layer.identity.name = "Background";
            
            if (comp.background->is_color()) {
                bg_layer.identity.type = LayerType::Solid;
                auto color = comp.background->get_color();
                if (color) {
                    bg_layer.text.fill_color = *color;
                }
                bg_layer.source = EmptySource{};
            } else {
                bg_layer.identity.type = LayerType::Image;
                bg_layer.source = MediaSource{ { comp.background->value } };
            }

            bg_layer.transform.width = comp.width;
            bg_layer.transform.height = comp.height;
            bg_layer.playback.timing.start = 0.0;
            bg_layer.playback.timing.duration = comp.duration;
            
            // Background is always at the bottom
            comp.layers.insert(comp.layers.begin(), std::move(bg_layer));
        }

        for (auto& layer : comp.layers) {
            // Validation is now implicit via std::variant structure.
            // We just ensure it's not EmptySource for critical layers if needed.
        }
    }
}

} // namespace tachyon::core
