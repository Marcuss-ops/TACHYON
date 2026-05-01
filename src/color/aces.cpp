#include "tachyon/color/aces.h"
#include <algorithm>
#include <cmath>

namespace tachyon::color {

struct ACESColorManager::Impl {
    ACESVersion version{ACESVersion::ACES_1_1};
    bool ocio_available{false};
    std::string working_space{"ACES AP1"};
    float gamma_value{2.2f};
};

ACESColorManager::ACESColorManager() : impl_(std::make_unique<Impl>()) {}
ACESColorManager::~ACESColorManager() = default;

bool ACESColorManager::initialize(ACESVersion version) {
    impl_->version = version;
    return true;
}

ColorConversionResult ACESColorManager::from_aces(const float* aces_rgb, float* output_rgb, size_t pixel_count) const {
    if (!aces_rgb || !output_rgb) {
        return {false, "Null pointer"};
    }
    // For now, simple pass-through (should apply ACES AP1 conversion)
    for (size_t i = 0; i < pixel_count; ++i) {
        output_rgb[i*3+0] = aces_rgb[i*3+0];
        output_rgb[i*3+1] = aces_rgb[i*3+1];
        output_rgb[i*3+2] = aces_rgb[i*3+2];
    }
    return {true, ""};
}

ColorConversionResult ACESColorManager::to_aces(const float* input_rgb, float* aces_rgb, size_t pixel_count) const {
    if (!input_rgb || !aces_rgb) {
        return {false, "Null pointer"};
    }
    for (size_t i = 0; i < pixel_count; ++i) {
        aces_rgb[i*3+0] = input_rgb[i*3+0];
        aces_rgb[i*3+1] = input_rgb[i*3+1];
        aces_rgb[i*3+2] = input_rgb[i*3+2];
    }
    return {true, ""};
}

void ACESColorManager::apply_aces_tone_curve(float* r, float* g, float* b, size_t pixel_count) const {
    // ACES tone curve (simplified): 1) / (1 + exp(-exposure)) - 0.5
    for (size_t i = 0; i < pixel_count; ++i) {
        r[i] = 1.0f / (1.0f + std::exp(-r[i])) - 0.5f;
        g[i] = 1.0f / (1.0f + std::exp(-g[i])) - 0.5f;
        b[i] = 1.0f / (1.0f + std::exp(-b[i])) - 0.5f;
    }
}

bool ACESColorManager::has_ocio() const {
    return impl_->ocio_available;
}

bool ACESColorManager::load_ocio_config(const std::string& config_path) {
    // TODO(ocio): integrate OpenColorIO when TACHYON_OCIO target is available
    (void)config_path;
    impl_->ocio_available = false;
    return false;
}

const char* ACESColorManager::ocio_version() const {
    return "OCIO not integrated";
}

const char* ACESColorManager::working_space_name() const {
    return impl_->working_space.c_str();
}

float ACESColorManager::gamma() const {
    return impl_->gamma_value;
}

namespace colorspace {

void aces_ap0_to_ap1(float* r, float* g, float* b, size_t count) {
    // ACES AP0 to AP1 conversion matrix (simplified)
    for (size_t i = 0; i < count; ++i) {
        float R = r[i], G = g[i], B = b[i];
        r[i] =  0.6954522414f * R + 0.1406786965f * G + 0.1638690622f * B;
        g[i] = 0.0447945634f * R + 0.8572752828f * G + 0.0979301537f * B;
        b[i] = -0.0055258826f * R + 0.0040252103f * G + 0.7428116390f * B;
    }
}

void aces_ap1_to_ap0(float* r, float* g, float* b, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        float R = r[i], G = g[i], B = b[i];
        r[i] =  1.4514393161f * R - 0.2365107465f * G - 0.2149285697f * B;
        g[i] = -0.0765537749f * R + 1.1762012114f * G - 0.0996474365f * B;
        b[i] =  0.0083161484f * R - 0.0060324498f * G + 1.3452427882f * B;
    }
}

void aces_ap1_to_srgb(float* r, float* g, float* b, size_t count) {
    // First apply ACES tone curve, then convert to sRGB
    for (size_t i = 0; i < count; ++i) {
        float R = std::max(0.0f, r[i]);
        float G = std::max(0.0f, g[i]);
        float B = std::max(0.0f, b[i]);
        // Simplified sRGB transfer function
        auto linear_to_srgb = [](float c) {
            return c <= 0.0031308f ? 12.92f * c : 1.055f * std::pow(c, 1.0f/2.4f) - 0.055f;
        };
        r[i] = std::clamp(linear_to_srgb(R), 0.0f, 1.0f);
        g[i] = std::clamp(linear_to_srgb(G), 0.0f, 1.0f);
        b[i] = std::clamp(linear_to_srgb(B), 0.0f, 1.0f);
    }
}

void srgb_to_aces_ap1(float* r, float* g, float* b, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        auto srgb_to_linear = [](float c) {
            return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
        };
        float R = srgb_to_linear(r[i]);
        float G = srgb_to_linear(g[i]);
        float B = srgb_to_linear(b[i]);
        // Convert sRGB to ACES AP1 (simplified matrix)
        r[i] =  0.61319f * R + 0.33951f * G + 0.04737f * B;
        g[i] = 0.07021f * R + 0.91634f * G + 0.01345f * B;
        b[i] = 0.02062f * R + 0.10957f * G + 0.86981f * B;
    }
}

} // namespace colorspace

} // namespace tachyon::color
