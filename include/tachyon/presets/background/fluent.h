#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/core/types/colors.h"
#include "tachyon/presets/background/procedural.h"
#include <string>
#include <functional>

namespace tachyon::presets::background {

/**
 * @brief Fluent builder for procedural backgrounds (Remotion-style API).
 * 
 * Usage:
 * @code
 * auto bg = aura()
 *     .width(1920).height(1080)
 *     .duration(8.0)
 *     .seed(42)
 *     .palette(palettes::neon_night())
 *     .grain(0.12)
 *     .build();
 * @endcode
 */
class BackgroundBuilder {
    procedural_bg::ProceduralParams params_;
    std::string kind_{"aura"};
    int width_{1920};
    int height_{1080};
    double duration_{5.0};
    std::string id_{"bg_procedural"};
    
public:
    explicit BackgroundBuilder(std::string kind = "aura") : kind_(std::move(kind)) {}
    
    BackgroundBuilder& width(int w) { width_ = w; return *this; }
    BackgroundBuilder& height(int h) { height_ = h; return *this; }
    BackgroundBuilder& size(int w, int h) { width_ = w; height_ = h; return *this; }
    BackgroundBuilder& duration(double d) { duration_ = d; return *this; }
    BackgroundBuilder& seed(uint64_t s) { params_.seed = s; return *this; }
    BackgroundBuilder& grain(double g) { params_.grain = g; return *this; }
    BackgroundBuilder& contrast(double c) { params_.contrast = c; return *this; }
    BackgroundBuilder& softness(double s) { params_.softness = s; return *this; }
    BackgroundBuilder& motion_speed(double s) { params_.motion_speed = s; return *this; }
    
    BackgroundBuilder& palette(const procedural_bg::ProceduralParams& p) {
        params_.palette_a = p.palette_a;
        params_.palette_b = p.palette_b;
        params_.palette_c = p.palette_c;
        params_.motion_speed = p.motion_speed;
        params_.contrast = p.contrast;
        params_.grain = p.grain;
        params_.softness = p.softness;
        params_.seed = p.seed;
        return *this;
    }
    
    BackgroundBuilder& id(std::string id) { id_ = std::move(id); return *this; }
    
    [[nodiscard]] LayerSpec build() const {
        if (kind_ == "aura") {
            return procedural_bg::aura(width_, height_, params_, duration_);
        } else if (kind_ == "shapegrid" || kind_ == "grid") {
            return procedural_bg::shapegrid(width_, height_, params_, duration_);
        } else if (kind_ == "stars") {
            return procedural_bg::stars(width_, height_, params_, duration_);
        } else if (kind_ == "noise") {
            return procedural_bg::noise(width_, height_, params_, duration_);
        } else if (kind_ == "grid_lines") {
            return procedural_bg::modern_tech_grid(width_, height_, params_, 60.0, duration_);
        } else if (kind_ == "classico") {
            return procedural_bg::classico_premium(width_, height_, params_, duration_);
        }
        // Default to aura
        return procedural_bg::aura(width_, height_, params_, duration_);
    }
    
    [[nodiscard]] operator LayerSpec() const { return build(); }
};

// ---------------------------------------------------------------------------
// Factory functions for Remotion-style background creation
// ---------------------------------------------------------------------------

inline BackgroundBuilder aura(const procedural_bg::ProceduralParams& params = procedural_bg::palettes::neon_night()) {
    BackgroundBuilder b("aura");
    b.palette(params);
    return b;
}

inline BackgroundBuilder grid(const procedural_bg::ProceduralParams& params = procedural_bg::palettes::dark_tech()) {
    BackgroundBuilder b("grid");
    b.palette(params);
    return b;
}

inline BackgroundBuilder shapegrid(const procedural_bg::ProceduralParams& params = procedural_bg::palettes::dark_tech()) {
    BackgroundBuilder b("shapegrid");
    b.palette(params);
    return b;
}

inline BackgroundBuilder stars(const procedural_bg::ProceduralParams& params = procedural_bg::palettes::cool_mint()) {
    BackgroundBuilder b("stars");
    b.palette(params);
    return b;
}

inline BackgroundBuilder noise(const procedural_bg::ProceduralParams& params = procedural_bg::palettes::dark_tech()) {
    BackgroundBuilder b("noise");
    b.palette(params);
    return b;
}

inline BackgroundBuilder grid_lines(const procedural_bg::ProceduralParams& params = procedural_bg::palettes::neon_grid()) {
    BackgroundBuilder b("grid_lines");
    b.palette(params);
    return b;
}

// ---------------------------------------------------------------------------
// Solid color background helper
// ---------------------------------------------------------------------------

inline LayerSpec solid(const ColorSpec& color, int width = 1920, int height = 1080, double duration = 5.0) {
    LayerSpec bg;
    bg.id = "bg_solid";
    bg.name = "Solid Background";
    bg.type = "solid";
    bg.enabled = true;
    bg.visible = true;
    bg.in_point = 0.0;
    bg.out_point = duration;
    bg.width = width;
    bg.height = height;
    bg.opacity = 1.0;
    bg.fill_color = AnimatedColorSpec(color);
    return bg;
}

// ---------------------------------------------------------------------------
// Image background helper  
// ---------------------------------------------------------------------------

inline LayerSpec image(std::string path, int width = 1920, int height = 1080, double duration = 5.0) {
    LayerSpec bg;
    bg.id = "bg_image";
    bg.name = "Image Background";
    bg.type = "image";
    bg.enabled = true;
    bg.visible = true;
    bg.in_point = 0.0;
    bg.out_point = duration;
    bg.width = width;
    bg.height = height;
    bg.opacity = 1.0;
    bg.asset_id = std::move(path);
    return bg;
}

// ---------------------------------------------------------------------------
// Video background helper
// ---------------------------------------------------------------------------

inline LayerSpec video(std::string path, int width = 1920, int height = 1080, double duration = 5.0, double playback_speed = 1.0) {
    LayerSpec bg;
    bg.id = "bg_video";
    bg.name = "Video Background";
    bg.type = "video";
    bg.enabled = true;
    bg.visible = true;
    bg.in_point = 0.0;
    bg.out_point = duration;
    bg.width = width;
    bg.height = height;
    bg.opacity = 1.0;
    bg.asset_id = std::move(path);
    return bg;
}

} // namespace tachyon::presets::background
