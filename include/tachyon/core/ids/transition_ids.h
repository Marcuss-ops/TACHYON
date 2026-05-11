#pragma once

#include <string_view>

namespace tachyon::ids::transition {

// Common Transitions
inline constexpr std::string_view crossfade = "tachyon.transition.crossfade";
inline constexpr std::string_view fade = "tachyon.transition.fade";
inline constexpr std::string_view slide = "tachyon.transition.slide";
inline constexpr std::string_view slide_left = "tachyon.transition.slide_left";
inline constexpr std::string_view slide_right = "tachyon.transition.slide_right";
inline constexpr std::string_view luma_dissolve = "tachyon.transition.luma_dissolve";
inline constexpr std::string_view zoom = "tachyon.transition.zoom";
inline constexpr std::string_view flip = "tachyon.transition.flip";
inline constexpr std::string_view blur = "tachyon.transition.blur";
inline constexpr std::string_view fade_to_black = "tachyon.transition.fade_to_black";
inline constexpr std::string_view wipe_linear = "tachyon.transition.wipe_linear";
inline constexpr std::string_view wipe_angular = "tachyon.transition.wipe_angular";
inline constexpr std::string_view push_left = "tachyon.transition.push_left";
inline constexpr std::string_view slide_easing = "tachyon.transition.slide_easing";
inline constexpr std::string_view slide_up = "tachyon.transition.slide_up";
inline constexpr std::string_view swipe_left = "tachyon.transition.swipe_left";
inline constexpr std::string_view circle_iris = "tachyon.transition.circle_iris";
inline constexpr std::string_view glitch_slice = "tachyon.transition.glitch_slice";
inline constexpr std::string_view pixelate = "tachyon.transition.pixelate";
inline constexpr std::string_view rgb_split = "tachyon.transition.rgb_split";
inline constexpr std::string_view kaleidoscope = "tachyon.transition.kaleidoscope";
inline constexpr std::string_view ripple = "tachyon.transition.ripple";
inline constexpr std::string_view spin = "tachyon.transition.spin";
inline constexpr std::string_view zoom_blur = "tachyon.transition.zoom_blur";
inline constexpr std::string_view zoom_in = "tachyon.transition.zoom_in";
inline constexpr std::string_view directional_blur_wipe = "tachyon.transition.directional_blur_wipe";
inline constexpr std::string_view flash = "tachyon.transition.flash";

// Specialized / Modern V2
inline constexpr std::string_view flash_cut = "tachyon.transition.flash_cut";
inline constexpr std::string_view smooth_wipe = "tachyon.transition.smooth_wipe";
inline constexpr std::string_view soft_zoom_blur = "tachyon.transition.soft_zoom_blur";

} // namespace tachyon::ids::transition
