#pragma once

#include "tachyon/presets/image/image_params.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"

namespace tachyon::presets {

[[nodiscard]] LayerSpec build_image(const ImageParams& p);          // dispatcher su p.animation

[[nodiscard]] LayerSpec build_image_static(const ImageParams& p);
[[nodiscard]] LayerSpec build_image_slow_zoom(const ImageParams& p);
[[nodiscard]] LayerSpec build_image_pan_left(const ImageParams& p);
[[nodiscard]] LayerSpec build_image_pan_right(const ImageParams& p);
[[nodiscard]] LayerSpec build_image_pan_up(const ImageParams& p);
[[nodiscard]] LayerSpec build_image_pan_down(const ImageParams& p);
[[nodiscard]] LayerSpec build_image_ken_burns(const ImageParams& p);

} // namespace tachyon::presets
