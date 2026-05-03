#include "light_leak_presets.h"

namespace tachyon::renderer2d {

LightLeakSettings make_light_leak_preset(LightLeakPreset p) {
    LightLeakSettings s;
    switch(p) {
        case LightLeakPreset::AmberSweepClassic:
            s.colorA = {1.0f, 0.55f, 0.08f, 1.0f}; s.colorB = {1.0f, 0.76f, 0.27f, 1.0f};
            s.intensity = 0.88f; s.width = 0.28f; s.blur = 28.0f;
            break;
        case LightLeakPreset::AmberFilmBurn:
            s.colorA = {1.0f, 0.32f, 0.0f, 1.0f}; s.colorB = {1.0f, 0.67f, 0.14f, 1.0f};
            s.intensity = 1.05f; s.width = 0.22f; s.blur = 34.0f;
            break;
        case LightLeakPreset::GoldenSoftLeak:
            s.colorA = {1.0f, 0.67f, 0.22f, 1.0f}; s.colorB = {1.0f, 0.86f, 0.47f, 1.0f};
            s.intensity = 0.55f; s.width = 0.36f; s.blur = 42.0f;
            break;
        case LightLeakPreset::AmberEdgeLeak:
            s.colorA = {1.0f, 0.39f, 0.04f, 1.0f}; s.colorB = {1.0f, 0.73f, 0.25f, 1.0f};
            s.intensity = 0.75f; s.width = 0.18f; s.blur = 36.0f;
            s.posOffset = -0.4f;
            break;
        case LightLeakPreset::AmberDoubleSweep:
            s.colorA = {1.0f, 0.47f, 0.0f, 1.0f}; s.colorB = {1.0f, 0.82f, 0.35f, 1.0f};
            s.intensity = 0.85f; s.width = 0.28f; s.blur = 30.0f;
            s.doubleBand = true;
            break;
        case LightLeakPreset::AmberAnamorphicStreak:
            s.colorA = {1.0f, 0.57f, 0.1f, 1.0f}; s.colorB = {1.0f, 0.9f, 0.51f, 1.0f};
            s.intensity = 0.95f; s.width = 0.08f; s.blur = 48.0f;
            s.angle = -8.0f;
            break;
        case LightLeakPreset::VintageAmberGate:
            s.colorA = {0.9f, 0.35f, 0.04f, 1.0f}; s.colorB = {1.0f, 0.71f, 0.27f, 1.0f};
            s.intensity = 0.82f; s.width = 0.30f; s.blur = 30.0f;
            s.vintage = true;
            break;
        case LightLeakPreset::WarmFlashLeak:
            s.colorA = {1.0f, 0.53f, 0.08f, 1.0f}; s.colorB = {1.0f, 0.92f, 0.59f, 1.0f};
            s.intensity = 1.15f; s.width = 0.42f; s.blur = 56.0f;
            break;
        case LightLeakPreset::DocumentaryAmberSubtle:
            s.colorA = {1.0f, 0.59f, 0.18f, 1.0f}; s.colorB = {1.0f, 0.8f, 0.39f, 1.0f};
            s.intensity = 0.28f; s.width = 0.45f; s.blur = 64.0f;
            break;
        case LightLeakPreset::HotCoreAmber:
            s.colorA = {1.0f, 0.41f, 0.0f, 1.0f}; s.colorB = {1.0f, 0.9f, 0.43f, 1.0f};
            s.intensity = 0.90f; s.width = 0.26f; s.blur = 32.0f;
            s.hotCore = true;
            break;
    }
    return s;
}

} // namespace tachyon::renderer2d
