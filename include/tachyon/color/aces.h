#pragma once

#include "tachyon/renderer2d/color/color_profile.h"
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

namespace tachyon::color {

// ACES color space identifiers
enum class ACESVersion {
    ACES_1_0,
    ACES_1_1,
    ACES_2_0
};

// Color space conversion results
struct ColorConversionResult {
    bool success{false};
    std::string error;
};

// Advanced Color Management System supporting ACES and OCIO
class ACESColorManager {
public:
    ACESColorManager();
    ~ACESColorManager();

    // Initialize with ACES version
    bool initialize(ACESVersion version = ACESVersion::ACES_1_1);

    // Convert from ACES color space to working space
    ColorConversionResult from_aces(const float* aces_rgb, float* output_rgb, size_t pixel_count) const;

    // Convert from working space to ACES color space
    ColorConversionResult to_aces(const float* input_rgb, float* aces_rgb, size_t pixel_count) const;

    // Apply ACES tone mapping (ACES curve)
    void apply_aces_tone_curve(float* r, float* g, float* b, size_t pixel_count) const;

    // OCIO integration (if available)
    bool has_ocio() const;
    bool load_ocio_config(const std::string& config_path);
    const char* ocio_version() const;

    // Get current color space info
    const char* working_space_name() const;
    float gamma() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Utility functions for color space conversions
namespace colorspace {

    // ACES AP0 (wide gamut) to ACES AP1 (working space)
    void aces_ap0_to_ap1(float* r, float* g, float* b, size_t count);
    
    // ACES AP1 to AP0
    void aces_ap1_to_ap0(float* r, float* g, float* b, size_t count);
    
    // ACES AP1 to sRGB (with tone mapping)
    void aces_ap1_to_srgb(float* r, float* g, float* b, size_t count);
    
    // sRGB to ACES AP1
    void srgb_to_aces_ap1(float* r, float* g, float* b, size_t count);

} // namespace colorspace

} // namespace tachyon::color
