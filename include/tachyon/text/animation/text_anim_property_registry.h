#pragma once

#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/text/core/layout/resolved_text_layout.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/text/animation/text_animator_utils.h"
#include <string>
#include <vector>
#include <functional>
#include <variant>

namespace tachyon::text {

enum class TextAnimPropertyId {
    Position,
    Scale,
    Rotation,
    Opacity,
    Fill,
    Stroke,
    StrokeWidth,
    Blur,
    Reveal,
    Tracking
};

struct TextAnimPropertyValue {
    TextAnimPropertyId id;
    std::variant<double, math::Vector2, ColorSpec> value;
};

struct TextAnimPropertyDescriptor {
    TextAnimPropertyId id;
    std::string name;
    
    bool supports_scalar;
    bool supports_simd;
    bool supports_parallel;
    
    using SampleFn = TextAnimPropertyValue(*)(const TextAnimatorPropertySpec&, float t);
    using ApplyToGlyphFn = void(*)(PositionedGlyph& glyph, const TextAnimatorPropertySpec& spec, float staggered_t, float coverage);
    using ApplyBatchFn = void(*)(std::span<PositionedGlyph> glyphs, const TextAnimatorPropertySpec& spec, std::span<float> staggered_ts, std::span<float> coverages);
    
    SampleFn sample;
    ApplyToGlyphFn apply_to_glyph;
    ApplyBatchFn apply_batch;
};

struct ResolvedTextAnimProperty {
    const TextAnimPropertyDescriptor* descriptor;
    const TextAnimatorPropertySpec* spec;
    
    void apply_to_glyph(PositionedGlyph& glyph, float staggered_t, float coverage) const {
        if (descriptor && descriptor->apply_to_glyph) {
            descriptor->apply_to_glyph(glyph, *spec, staggered_t, coverage);
        }
    }
    
    void apply_batch(std::span<PositionedGlyph> glyphs, std::span<float> staggered_ts, std::span<float> coverages) const {
        if (descriptor && descriptor->apply_batch) {
            descriptor->apply_batch(glyphs, *spec, staggered_ts, coverages);
        }
    }
};

class TextAnimPropertyRegistry {
public:
    static TextAnimPropertyRegistry& instance() {
        static TextAnimPropertyRegistry registry;
        return registry;
    }
    
    void register_property(const TextAnimPropertyDescriptor& desc) {
        properties_.push_back(desc);
    }
    
    const TextAnimPropertyDescriptor* find(TextAnimPropertyId id) const {
        for (const auto& prop : properties_) {
            if (prop.id == id) return &prop;
        }
        return nullptr;
    }
    
    std::vector<ResolvedTextAnimProperty> resolve_properties(const TextAnimatorPropertySpec& spec) const {
        std::vector<ResolvedTextAnimProperty> resolved;
        for (const auto& prop : properties_) {
            if (is_property_active(prop.id, spec)) {
                resolved.push_back({&prop, &spec});
            }
        }
        return resolved;
    }
    
private:
    TextAnimPropertyRegistry() {
        register_property({
            TextAnimPropertyId::Position,
            "position_offset",
            true, true, true,
            nullptr,
            &apply_position,
            &apply_batch_position
        });
        
        register_property({
            TextAnimPropertyId::Scale,
            "scale",
            true, true, true,
            nullptr,
            &apply_scale,
            &apply_batch_scale
        });
        
        register_property({
            TextAnimPropertyId::Rotation,
            "rotation",
            true, false, true,
            nullptr,
            &apply_rotation,
            &apply_batch_rotation
        });
        
        register_property({
            TextAnimPropertyId::Opacity,
            "opacity",
            true, true, true,
            nullptr,
            &apply_opacity,
            &apply_batch_opacity
        });
        
        register_property({
            TextAnimPropertyId::Fill,
            "fill_color",
            true, false, true,
            nullptr,
            &apply_fill,
            &apply_batch_fill
        });
        
        register_property({
            TextAnimPropertyId::Stroke,
            "stroke_color",
            true, false, true,
            nullptr,
            &apply_stroke,
            &apply_batch_stroke
        });
        
        register_property({
            TextAnimPropertyId::StrokeWidth,
            "stroke_width",
            true, false, true,
            nullptr,
            &apply_stroke_width,
            &apply_batch_stroke_width
        });
        
        register_property({
            TextAnimPropertyId::Blur,
            "blur_radius",
            true, false, true,
            nullptr,
            &apply_blur,
            &apply_batch_blur
        });
        
        register_property({
            TextAnimPropertyId::Reveal,
            "reveal",
            true, false, true,
            nullptr,
            &apply_reveal,
            &apply_batch_reveal
        });
        
        register_property({
            TextAnimPropertyId::Tracking,
            "tracking_amount",
            true, false, true,
            nullptr,
            &apply_tracking,
            &apply_batch_tracking
        });
    }
    
    bool is_property_active(TextAnimPropertyId id, const TextAnimatorPropertySpec& spec) const {
        switch (id) {
            case TextAnimPropertyId::Position:
                return spec.position_offset_value.has_value() || !spec.position_offset_keyframes.empty();
            case TextAnimPropertyId::Scale:
                return spec.scale_value.has_value() || !spec.scale_keyframes.empty();
            case TextAnimPropertyId::Rotation:
                return spec.rotation_value.has_value() || !spec.rotation_keyframes.empty();
            case TextAnimPropertyId::Opacity:
                return spec.opacity_value.has_value() || !spec.opacity_keyframes.empty();
            case TextAnimPropertyId::Fill:
                return spec.fill_color_value.has_value() || !spec.fill_color_keyframes.empty();
            case TextAnimPropertyId::Stroke:
                return spec.stroke_color_value.has_value() || !spec.stroke_color_keyframes.empty();
            case TextAnimPropertyId::StrokeWidth:
                return spec.stroke_width_value.has_value() || !spec.stroke_width_keyframes.empty();
            case TextAnimPropertyId::Blur:
                return spec.blur_radius_value.has_value() || !spec.blur_radius_keyframes.empty();
            case TextAnimPropertyId::Reveal:
                return spec.reveal_value.has_value() || !spec.reveal_keyframes.empty();
            case TextAnimPropertyId::Tracking:
                return spec.tracking_amount_value.has_value() || !spec.tracking_amount_keyframes.empty();
            default:
                return false;
        }
    }
    
    static void apply_position(PositionedGlyph& glyph, const TextAnimatorPropertySpec& spec, float staggered_t, float coverage) {
        math::Vector2 offset = sample_vector2_kfs(spec.position_offset_value, spec.position_offset_keyframes, staggered_t);
        glyph.x += static_cast<std::int32_t>(std::lround(offset.x * coverage));
        glyph.y += static_cast<std::int32_t>(std::lround(offset.y * coverage));
    }
    
    static void apply_scale(PositionedGlyph& glyph, const TextAnimatorPropertySpec& spec, float staggered_t, float coverage) {
        double scale_val = sample_scalar_kfs(spec.scale_value, spec.scale_keyframes, staggered_t);
        float fscale = static_cast<float>(scale_val);
        glyph.scale.x = glyph.scale.x * (1.0f - coverage) + (glyph.scale.x * fscale) * coverage;
        glyph.scale.y = glyph.scale.y * (1.0f - coverage) + (glyph.scale.y * fscale) * coverage;
    }
    
    static void apply_rotation(PositionedGlyph& glyph, const TextAnimatorPropertySpec& spec, float staggered_t, float coverage) {
        double rot = sample_scalar_kfs(spec.rotation_value, spec.rotation_keyframes, staggered_t);
        glyph.rotation += static_cast<float>(rot) * coverage;
    }
    
    static void apply_opacity(PositionedGlyph& glyph, const TextAnimatorPropertySpec& spec, float staggered_t, float coverage) {
        double op = sample_scalar_kfs(spec.opacity_value, spec.opacity_keyframes, staggered_t);
        float fop = static_cast<float>(op);
        glyph.opacity = std::clamp(glyph.opacity * (1.0f - coverage) + fop * coverage, 0.0f, 1.0f);
    }
    
    static void apply_fill(PositionedGlyph& glyph, const TextAnimatorPropertySpec& spec, float staggered_t, float coverage) {
        ColorSpec target = sample_color_kfs(spec.fill_color_value, spec.fill_color_keyframes, staggered_t);
        glyph.fill_color = blend_color(glyph.fill_color, target, coverage);
    }
    
    static void apply_stroke(PositionedGlyph& glyph, const TextAnimatorPropertySpec& spec, float staggered_t, float coverage) {
        ColorSpec target = sample_color_kfs(spec.stroke_color_value, spec.stroke_color_keyframes, staggered_t);
        glyph.stroke_color = blend_color(glyph.stroke_color, target, coverage);
    }
    
    static void apply_stroke_width(PositionedGlyph& glyph, const TextAnimatorPropertySpec& spec, float staggered_t, float coverage) {
        double width = sample_scalar_kfs(spec.stroke_width_value, spec.stroke_width_keyframes, staggered_t);
        float fwidth = static_cast<float>(width);
        glyph.stroke_width = glyph.stroke_width * (1.0f - coverage) + fwidth * coverage;
    }
    
    static void apply_blur(PositionedGlyph& glyph, const TextAnimatorPropertySpec& spec, float staggered_t, float coverage) {
        double radius = sample_scalar_kfs(spec.blur_radius_value, spec.blur_radius_keyframes, staggered_t);
        float fradius = static_cast<float>(radius);
        glyph.blur_radius = std::max(0.0f, glyph.blur_radius * (1.0f - coverage) + fradius * coverage);
    }
    
    static void apply_reveal(PositionedGlyph& glyph, const TextAnimatorPropertySpec& spec, float staggered_t, float coverage) {
        double reveal = sample_scalar_kfs(spec.reveal_value, spec.reveal_keyframes, staggered_t);
        float freveal = static_cast<float>(reveal);
        glyph.reveal_factor = std::clamp(glyph.reveal_factor * (1.0f - coverage) + freveal * coverage, 0.0f, 1.0f);
    }
    
    static void apply_tracking(PositionedGlyph& glyph, const TextAnimatorPropertySpec& spec, float staggered_t, float coverage) {
        (void)glyph; (void)spec; (void)staggered_t; (void)coverage;
    }
    
    static void apply_batch_position(std::span<PositionedGlyph> glyphs, const TextAnimatorPropertySpec& spec, std::span<float> staggered_ts, std::span<float> coverages) {
        for (size_t i = 0; i < glyphs.size() && i < staggered_ts.size() && i < coverages.size(); ++i) {
            if (coverages[i] <= 0.0f) continue;
            math::Vector2 offset = sample_vector2_kfs(spec.position_offset_value, spec.position_offset_keyframes, staggered_ts[i]);
            glyphs[i].x += static_cast<std::int32_t>(std::lround(offset.x * coverages[i]));
            glyphs[i].y += static_cast<std::int32_t>(std::lround(offset.y * coverages[i]));
        }
    }
    
    static void apply_batch_scale(std::span<PositionedGlyph> glyphs, const TextAnimatorPropertySpec& spec, std::span<float> staggered_ts, std::span<float> coverages) {
        for (size_t i = 0; i < glyphs.size() && i < staggered_ts.size() && i < coverages.size(); ++i) {
            if (coverages[i] <= 0.0f) continue;
            double scale_val = sample_scalar_kfs(spec.scale_value, spec.scale_keyframes, staggered_ts[i]);
            float fscale = static_cast<float>(scale_val);
            glyphs[i].scale.x = glyphs[i].scale.x * (1.0f - coverages[i]) + (glyphs[i].scale.x * fscale) * coverages[i];
            glyphs[i].scale.y = glyphs[i].scale.y * (1.0f - coverages[i]) + (glyphs[i].scale.y * fscale) * coverages[i];
        }
    }
    
    static void apply_batch_rotation(std::span<PositionedGlyph> glyphs, const TextAnimatorPropertySpec& spec, std::span<float> staggered_ts, std::span<float> coverages) {
        for (size_t i = 0; i < glyphs.size() && i < staggered_ts.size() && i < coverages.size(); ++i) {
            if (coverages[i] <= 0.0f) continue;
            double rot = sample_scalar_kfs(spec.rotation_value, spec.rotation_keyframes, staggered_ts[i]);
            glyphs[i].rotation += static_cast<float>(rot) * coverages[i];
        }
    }
    
    static void apply_batch_opacity(std::span<PositionedGlyph> glyphs, const TextAnimatorPropertySpec& spec, std::span<float> staggered_ts, std::span<float> coverages) {
        for (size_t i = 0; i < glyphs.size() && i < staggered_ts.size() && i < coverages.size(); ++i) {
            if (coverages[i] <= 0.0f) continue;
            double op = sample_scalar_kfs(spec.opacity_value, spec.opacity_keyframes, staggered_ts[i]);
            float fop = static_cast<float>(op);
            glyphs[i].opacity = std::clamp(glyphs[i].opacity * (1.0f - coverages[i]) + fop * coverages[i], 0.0f, 1.0f);
        }
    }
    
    static void apply_batch_fill(std::span<PositionedGlyph> glyphs, const TextAnimatorPropertySpec& spec, std::span<float> staggered_ts, std::span<float> coverages) {
        for (size_t i = 0; i < glyphs.size() && i < staggered_ts.size() && i < coverages.size(); ++i) {
            if (coverages[i] <= 0.0f) continue;
            ColorSpec target = sample_color_kfs(spec.fill_color_value, spec.fill_color_keyframes, staggered_ts[i]);
            glyphs[i].fill_color = blend_color(glyphs[i].fill_color, target, coverages[i]);
        }
    }
    
    static void apply_batch_stroke(std::span<PositionedGlyph> glyphs, const TextAnimatorPropertySpec& spec, std::span<float> staggered_ts, std::span<float> coverages) {
        for (size_t i = 0; i < glyphs.size() && i < staggered_ts.size() && i < coverages.size(); ++i) {
            if (coverages[i] <= 0.0f) continue;
            ColorSpec target = sample_color_kfs(spec.stroke_color_value, spec.stroke_color_keyframes, staggered_ts[i]);
            glyphs[i].stroke_color = blend_color(glyphs[i].stroke_color, target, coverages[i]);
        }
    }
    
    static void apply_batch_stroke_width(std::span<PositionedGlyph> glyphs, const TextAnimatorPropertySpec& spec, std::span<float> staggered_ts, std::span<float> coverages) {
        for (size_t i = 0; i < glyphs.size() && i < staggered_ts.size() && i < coverages.size(); ++i) {
            if (coverages[i] <= 0.0f) continue;
            double width = sample_scalar_kfs(spec.stroke_width_value, spec.stroke_width_keyframes, staggered_ts[i]);
            float fwidth = static_cast<float>(width);
            glyphs[i].stroke_width = glyphs[i].stroke_width * (1.0f - coverages[i]) + fwidth * coverages[i];
        }
    }
    
    static void apply_batch_blur(std::span<PositionedGlyph> glyphs, const TextAnimatorPropertySpec& spec, std::span<float> staggered_ts, std::span<float> coverages) {
        for (size_t i = 0; i < glyphs.size() && i < staggered_ts.size() && i < coverages.size(); ++i) {
            if (coverages[i] <= 0.0f) continue;
            double radius = sample_scalar_kfs(spec.blur_radius_value, spec.blur_radius_keyframes, staggered_ts[i]);
            float fradius = static_cast<float>(radius);
            glyphs[i].blur_radius = std::max(0.0f, glyphs[i].blur_radius * (1.0f - coverages[i]) + fradius * coverages[i]);
        }
    }
    
    static void apply_batch_reveal(std::span<PositionedGlyph> glyphs, const TextAnimatorPropertySpec& spec, std::span<float> staggered_ts, std::span<float> coverages) {
        for (size_t i = 0; i < glyphs.size() && i < staggered_ts.size() && i < coverages.size(); ++i) {
            if (coverages[i] <= 0.0f) continue;
            double reveal = sample_scalar_kfs(spec.reveal_value, spec.reveal_keyframes, staggered_ts[i]);
            float freveal = static_cast<float>(reveal);
            glyphs[i].reveal_factor = std::clamp(glyphs[i].reveal_factor * (1.0f - coverages[i]) + freveal * coverages[i], 0.0f, 1.0f);
        }
    }
    
    static void apply_batch_tracking(std::span<PositionedGlyph> glyphs, const TextAnimatorPropertySpec& spec, std::span<float> staggered_ts, std::span<float> coverages) {
        (void)glyphs; (void)spec; (void)staggered_ts; (void)coverages;
    }
    
    static ColorSpec blend_color(const ColorSpec& a, const ColorSpec& b, float t) {
        float clamped = std::clamp(t, 0.0f, 1.0f);
        auto lerp = [clamped](std::uint8_t x, std::uint8_t y) {
            return static_cast<std::uint8_t>(x + (y - x) * clamped);
        };
        return {lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), lerp(a.a, b.a)};
    }
    
    std::vector<TextAnimPropertyDescriptor> properties_;
};

} // namespace tachyon::text
