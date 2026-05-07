#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/core/types/colors.h"

#include <string>
#include <vector>
#include <functional>

namespace tachyon::scene {

class LayerBuilder;

/**
 * @brief Builder for text-specific properties on a LayerSpec.
 */
class TACHYON_API TextBuilder {
    LayerBuilder& parent_;
public:
    explicit TextBuilder(LayerBuilder& parent) : parent_(parent) {}

    TextBuilder& content(std::string t);
    TextBuilder& font(std::string f);
    TextBuilder& font_size(double sz);
    TextBuilder& subtitle_path(std::string path);
    TextBuilder& animator(const TextAnimatorSpec& anim);
    TextBuilder& animators(std::vector<TextAnimatorSpec> anims);
    TextBuilder& highlight(const TextHighlightSpec& hl);
    TextBuilder& highlights(std::vector<TextHighlightSpec> hls);
    TextBuilder& animation_preset(const std::string& id);

    LayerBuilder& done();
};

} // namespace tachyon::scene
