#pragma once
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/effects/generators/light_leak_types.h"

namespace tachyon::renderer2d {

struct LightLeakSettings {
    Color colorA = {1.0f, 0.55f, 0.08f, 1.0f}; // {255, 140, 20}
    Color colorB = {1.0f, 0.76f, 0.27f, 1.0f}; // {255, 195, 70}
    float intensity = 0.88f;
    float width = 0.28f;
    float blur = 28.0f;
    float angle = 0.0f; // 0 = usa seed
    bool doubleBand = false;
    bool vintage = false;
    bool hotCore = false;
    float posOffset = 0.0f; // per edge leak
};

LightLeakSettings make_light_leak_preset(LightLeakPreset preset);

} // namespace tachyon::renderer2d
