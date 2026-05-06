#pragma once

#include <string_view>

namespace tachyon::ids {

namespace transition {
inline constexpr std::string_view crossfade = "tachyon.transition.crossfade";
inline constexpr std::string_view luma_dissolve = "tachyon.transition.luma_dissolve";
inline constexpr std::string_view slide_up = "tachyon.transition.slide_up";
inline constexpr std::string_view swipe_left = "tachyon.transition.swipe_left";
inline constexpr std::string_view circle_iris = "tachyon.transition.circle_iris";
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
