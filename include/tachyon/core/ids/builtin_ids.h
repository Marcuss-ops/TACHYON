#pragma once

#include <string_view>

namespace tachyon::ids {

namespace transition {
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
inline constexpr std::string_view light_leak = "tachyon.transition.light_leak";
inline constexpr std::string_view light_leak_solar = "tachyon.transition.light_leak_solar";
inline constexpr std::string_view light_leak_nebula = "tachyon.transition.light_leak_nebula";
inline constexpr std::string_view light_leak_sunset = "tachyon.transition.light_leak_sunset";
inline constexpr std::string_view light_leak_ghost = "tachyon.transition.light_leak_ghost";
inline constexpr std::string_view film_burn = "tachyon.transition.film_burn";
inline constexpr std::string_view lightleak_soft_warm_edge = "tachyon.transition.lightleak.soft_warm_edge";
inline constexpr std::string_view lightleak_golden_sweep = "tachyon.transition.lightleak.golden_sweep";
inline constexpr std::string_view lightleak_creamy_white = "tachyon.transition.lightleak.creamy_white";
inline constexpr std::string_view lightleak_dusty_archive = "tachyon.transition.lightleak.dusty_archive";
inline constexpr std::string_view lightleak_lens_flare_pass = "tachyon.transition.lightleak.lens_flare_pass";
inline constexpr std::string_view lightleak_amber_sweep = "tachyon.transition.lightleak.amber_sweep";
inline constexpr std::string_view lightleak_neon_pulse = "tachyon.transition.lightleak.neon_pulse";
inline constexpr std::string_view lightleak_prism_shatter = "tachyon.transition.lightleak.prism_shatter";
inline constexpr std::string_view lightleak_vintage_sepia = "tachyon.transition.lightleak.vintage_sepia";
}

namespace background {
inline constexpr std::string_view solid = "tachyon.background.solid";
inline constexpr std::string_view transparent = "tachyon.background.transparent";
inline constexpr std::string_view grid = "tachyon.background.grid";
inline constexpr std::string_view checkerboard = "tachyon.background.checkerboard";
}

namespace effect {
inline constexpr std::string_view transition_glsl = "tachyon.effect.transition.glsl";
}

} // namespace tachyon::ids
