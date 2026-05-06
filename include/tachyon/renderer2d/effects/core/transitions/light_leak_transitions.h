#pragma once

#include "tachyon/core/api.h"
#include "tachyon/renderer2d/core/framebuffer.h"

// TachyonRenderer2D is a STATIC library. 
// On Windows, we should NOT use __declspec(dllimport/dllexport) for static libraries
// unless we are specifically building them into a DLL and want to export them.
// For now, we'll keep it simple: no API macro for static renderer functions.
#define TACHYON_RENDERER2D_API

namespace tachyon::renderer2d {

// Cinematic light leak and film burn transitions.
// These are implemented in light_leak_transitions.cpp and registered via TransitionManifest.

TACHYON_RENDERER2D_API Color transition_light_leak(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);
TACHYON_RENDERER2D_API Color transition_light_leak_solar(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);
TACHYON_RENDERER2D_API Color transition_light_leak_nebula(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);
TACHYON_RENDERER2D_API Color transition_light_leak_sunset(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);
TACHYON_RENDERER2D_API Color transition_light_leak_ghost(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);
TACHYON_RENDERER2D_API Color transition_film_burn(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);

TACHYON_RENDERER2D_API Color transition_lightleak_soft_warm_edge(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);
TACHYON_RENDERER2D_API Color transition_lightleak_golden_sweep(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);
TACHYON_RENDERER2D_API Color transition_lightleak_creamy_white(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);
TACHYON_RENDERER2D_API Color transition_lightleak_dusty_archive(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);
TACHYON_RENDERER2D_API Color transition_lightleak_lens_flare_pass(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);
TACHYON_RENDERER2D_API Color transition_lightleak_amber_sweep(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);
TACHYON_RENDERER2D_API Color transition_lightleak_neon_pulse(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);
TACHYON_RENDERER2D_API Color transition_lightleak_prism_shatter(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);
TACHYON_RENDERER2D_API Color transition_lightleak_vintage_sepia(float u, float v, float t, const SurfaceRGBA& input, const SurfaceRGBA* to_surface);

TACHYON_RENDERER2D_API void register_light_leak_implementations();

} // namespace tachyon::renderer2d
