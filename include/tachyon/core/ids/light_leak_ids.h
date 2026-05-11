#pragma once

#include <string_view>

namespace tachyon::ids::transition {

// Basic Light Leaks
inline constexpr std::string_view light_leak = "tachyon.transition.light_leak";
inline constexpr std::string_view light_leak_solar = "tachyon.transition.light_leak_solar";
inline constexpr std::string_view light_leak_nebula = "tachyon.transition.light_leak_nebula";
inline constexpr std::string_view light_leak_sunset = "tachyon.transition.light_leak_sunset";
inline constexpr std::string_view light_leak_ghost = "tachyon.transition.light_leak_ghost";
inline constexpr std::string_view film_burn = "tachyon.transition.film_burn";

// Premium Light Leaks (Enhanced VFX)
inline constexpr std::string_view lightleak_soft_warm_edge = "tachyon.transition.lightleak.soft_warm_edge";
inline constexpr std::string_view lightleak_golden_sweep = "tachyon.transition.lightleak.golden_sweep";
inline constexpr std::string_view lightleak_creamy_white = "tachyon.transition.lightleak.creamy_white";
inline constexpr std::string_view lightleak_dusty_archive = "tachyon.transition.lightleak.dusty_archive";
inline constexpr std::string_view lightleak_lens_flare_pass = "tachyon.transition.lightleak.lens_flare_pass";
inline constexpr std::string_view lightleak_amber_sweep = "tachyon.transition.lightleak.amber_sweep";
inline constexpr std::string_view lightleak_neon_pulse = "tachyon.transition.lightleak.neon_pulse";
inline constexpr std::string_view lightleak_prism_shatter = "tachyon.transition.lightleak.prism_shatter";
inline constexpr std::string_view lightleak_vintage_sepia = "tachyon.transition.lightleak.vintage_sepia";

// Fluid dynamic light leak systems
inline constexpr std::string_view lightleak_organic_blobs = "tachyon.transition.lightleak.organic_blobs";
inline constexpr std::string_view lightleak_lava_flow = "tachyon.transition.lightleak.lava_flow";
inline constexpr std::string_view lightleak_liquid_fission = "tachyon.transition.lightleak.liquid_fission";
inline constexpr std::string_view lightleak_cosmic_swirl = "tachyon.transition.lightleak.cosmic_swirl";
inline constexpr std::string_view lightleak_cinematic_amber = "tachyon.transition.lightleak.cinematic_amber";
inline constexpr std::string_view lightleak_procedural_remotion = "tachyon.transition.lightleak.procedural_remotion";

} // namespace tachyon::ids::transition
