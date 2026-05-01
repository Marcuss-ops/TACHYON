#pragma once

#include "tachyon/presets/text/text_params.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/presets/text/fluent.h"
#include <string>

namespace tachyon::presets {

[[nodiscard]] LayerSpec build_text(const TextParams& p);

[[nodiscard]] LayerSpec build_text_fade_up(const TextParams& p);
[[nodiscard]] LayerSpec build_text_blur_to_focus(const TextParams& p);
[[nodiscard]] LayerSpec build_text_typewriter(const TextParams& p);
[[nodiscard]] LayerSpec build_text_slide_in(const TextParams& p);
[[nodiscard]] LayerSpec build_text_tracking_reveal(const TextParams& p);
[[nodiscard]] LayerSpec build_text_outline_to_solid(const TextParams& p);
[[nodiscard]] LayerSpec build_text_number_flip(const TextParams& p);
[[nodiscard]] LayerSpec build_text_word_by_word(const TextParams& p);
[[nodiscard]] LayerSpec build_text_split_line(const TextParams& p);
[[nodiscard]] LayerSpec build_text_soft_scale(const TextParams& p);
[[nodiscard]] LayerSpec build_text_curtain_box(const TextParams& p);
[[nodiscard]] LayerSpec build_text_fill_wipe(const TextParams& p);
[[nodiscard]] LayerSpec build_text_slide_mask(const TextParams& p);
[[nodiscard]] LayerSpec build_text_kinetic(const TextParams& p);
[[nodiscard]] LayerSpec build_text_pop(const TextParams& p);
[[nodiscard]] LayerSpec build_text_y_rotate(const TextParams& p);
[[nodiscard]] LayerSpec build_text_morphing(const TextParams& p);
[[nodiscard]] LayerSpec build_text_underline(const TextParams& p);
[[nodiscard]] LayerSpec build_text_brushed_metal_title(const TextParams& p);

[[nodiscard]] SceneSpec build_text_scene(const TextParams& text,
                                          const SceneParams& scene = {});

LayerSpec build_text_layer(const std::string& text,
                           const std::string& font_id = "",
                           double font_size = 72.0,
                           const std::string& animation_style = "fade_up",
                           double duration = 10.0);

} // namespace tachyon::presets
