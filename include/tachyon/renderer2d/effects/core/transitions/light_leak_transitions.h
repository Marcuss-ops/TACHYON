#pragma once

#include "tachyon/core/api.h"
#include "tachyon/renderer2d/core/framebuffer.h"

// TachyonRenderer2D is a STATIC library. 
// On Windows, we should NOT use __declspec(dllimport/dllexport) for static libraries
// unless we are specifically building them into a DLL and want to export them.
// For now, we'll keep it simple: no API macro for static renderer functions.
#define TACHYON_RENDERER2D_API

namespace tachyon {
class TransitionRegistry;
struct TransitionDescriptor;
}

namespace tachyon::renderer2d {

// Cinematic light leak and film burn transitions.
// These are implemented in light_leak_transitions.cpp and registered via TransitionManifest.

// Implementation resolving registry for light leak styles.

// Utility resolution helper
void resolve_light_leak_implementations(tachyon::TransitionDescriptor& descriptor);

} // namespace tachyon::renderer2d
