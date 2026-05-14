#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/core/types/colors.h"
#include "tachyon/presets/background/background_builders.h"
#include "tachyon/presets/background/procedural.h"
#include <string>
#include <functional>

namespace tachyon::presets::background {

/**
 * @brief Fluent builder for procedural backgrounds (Remotion-style API).
 */ 
class BackgroundBuilder {
    procedural_bg::ProceduralParams params_;
    std::string kind_{};
    int width_{1920};
    int height_{1080};
    double duration_{5.0};
    std::string id_{"bg_procedural"};
    
public:
    explicit BackgroundBuilder(std::string kind = {}) : kind_(std::move(kind)) {}
    
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
        BackgroundParams p;
        p.kind = kind_;
        p.w = static_cast<float>(width_);
        p.h = static_cast<float>(height_);
        p.out_point = duration_;
        p.seed = params_.seed;
        p.speed = static_cast<float>(params_.motion_speed);
        p.color_a = params_.palette_a;
        p.color_b = params_.palette_b;
        p.color_c = params_.palette_c;
        p.contrast = static_cast<float>(params_.contrast);
        p.grain_amount = static_cast<float>(params_.grain);
        p.softness = static_cast<float>(params_.softness);
        return build_background(p);
    }
    
    [[nodiscard]] operator LayerSpec() const { return build(); }
};

inline BackgroundBuilder kind_aura(const procedural_bg::ProceduralParams& params = procedural_bg::palettes::neon_night()) {
    BackgroundBuilder b("tachyon.background.kind.aura");
    b.palette(params);
    return b;
}

inline BackgroundBuilder kind_grid(const procedural_bg::ProceduralParams& params = procedural_bg::palettes::dark_tech()) {
    BackgroundBuilder b("tachyon.background.kind.grid");
    b.palette(params);
    return b;
}

inline BackgroundBuilder kind_stars(const procedural_bg::ProceduralParams& params = procedural_bg::palettes::cool_mint()) {
    BackgroundBuilder b("tachyon.background.kind.stars");
    b.palette(params);
    return b;
}

inline BackgroundBuilder kind_noise(const procedural_bg::ProceduralParams& params = procedural_bg::palettes::dark_tech()) {
    BackgroundBuilder b("tachyon.background.kind.noise");
    b.palette(params);
    return b;
}

// ---------------------------------------------------------------------------
// Solid color background helper
// ---------------------------------------------------------------------------

inline LayerSpec solid(const ColorSpec& color, int width = 1920, int height = 1080, double duration = 5.0) {
    LayerSpec bg;
    bg.identity.id = "bg_solid";
    bg.identity.name = "Solid Background";
    bg.identity.type = LayerType::Solid;
    bg.identity.enabled = true;
    bg.identity.visible = true;
    bg.playback.timing.source_in = 0.0;
    bg.playback.timing.source_out = duration;
    bg.playback.timing.duration = duration;
    bg.transform.width = width;
    bg.transform.height = height;
    bg.transform.opacity = 1.0;
    
    // For solids, we use a procedural spec with kind "solid"
    ProceduralSpec p;
    p.kind = "tachyon.background.kind.solid";
    p.color_a = AnimatedColorSpec(color);
    bg.source.procedural = std::move(p);
    
    return bg;
}

// ---------------------------------------------------------------------------
// Image background helper  
// ---------------------------------------------------------------------------

inline LayerSpec image(std::string path, int width = 1920, int height = 1080, double duration = 5.0) {
    LayerSpec bg;
    bg.identity.id = "bg_image";
    bg.identity.name = "Image Background";
    bg.identity.type = LayerType::Image;
    bg.identity.enabled = true;
    bg.identity.visible = true;
    bg.playback.timing.source_in = 0.0;
    bg.playback.timing.source_out = duration;
    bg.playback.timing.duration = duration;
    bg.transform.width = width;
    bg.transform.height = height;
    bg.transform.opacity = 1.0;
    bg.source.asset_id = std::move(path);
    return bg;
}

// ---------------------------------------------------------------------------
// Video background helper
// ---------------------------------------------------------------------------

inline LayerSpec video(std::string path, int width = 1920, int height = 1080, double duration = 5.0, double playback_speed = 1.0) {
    (void)playback_speed;
    LayerSpec bg;
    bg.identity.id = "bg_video";
    bg.identity.name = "Video Background";
    bg.identity.type = LayerType::Video;
    bg.identity.enabled = true;
    bg.identity.visible = true;
    bg.playback.timing.source_in = 0.0;
    bg.playback.timing.source_out = duration;
    bg.playback.timing.duration = duration;
    bg.transform.width = width;
    bg.transform.height = height;
    bg.transform.opacity = 1.0;
    bg.source.asset_id = std::move(path);
    return bg;
}

} // namespace tachyon::presets::background
