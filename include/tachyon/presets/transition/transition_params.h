#pragma once

#include "tachyon/presets/preset_base.h"
#include "tachyon/core/animation/easing.h"
#include <map>
#include <string>

namespace tachyon::presets {

// Valid transition IDs (registered in init_builtin_transitions):
//
// Base:
//   "fade"                  crossfade
//   "fade_to_black"         fade attraverso il nero
//   "zoom"                  zoom in/out
//   "zoom_in"               solo zoom in
//   "zoom_out"              solo zoom out
//   "zoom_blur"             zoom con motion blur
//   "blur"                  blur wipe
//
// Slide:
//   "slide_left"            scorre a sinistra
//   "slide_right"           scorre a destra
//   "slide_up"              scorre verso l'alto
//   "slide_down"            scorre verso il basso
//   "slide_easing"          slide con ease integrato
//   "push_left"             push da sinistra
//
// Wipe:
//   "wipe_linear"           wipe orizzontale
//   "wipe_angular"          wipe angolare
//   "directional_blur_wipe" wipe con blur direzionale
//
// Creative:
//   "flip"                  ribaltamento orizzontale
//   "spin"                  rotazione
//   "circle_iris"           iris circolare
//   "pixelate"              dissolve pixelato
//   "glitch_slice"          fette glitch
//   "rgb_split"             aberrazione cromatica
//   "luma_dissolve"         dissolve noise-based

struct TransitionParams : LayerParams {
    std::string id;
    double      duration{0.4};
    animation::EasingPreset easing{animation::EasingPreset::EaseOut};
    double      delay{0.0};
    std::string direction{"none"}; // "up", "down", "left", "right", "random"
    std::map<std::string, double> parameters;
};

} // namespace tachyon::presets
