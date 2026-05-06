#pragma once

#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/text/content/subtitle.h"
#include "tachyon/text/layout/layout.h"
#include "tachyon/presets/text/text_params.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"

#include <string>
#include <vector>
#include <optional>
#include <map>

namespace tachyon {

struct TextLayerSpec {
    // Identity
    std::string id{"text_layer"};
    std::string name{"Text"};

    // Timing
    double in_point{0.0};
    double out_point{10.0};
    double opacity{1.0};

    // Position & dimensions
    double x{0.0};
    double y{0.0};
    double width{1920.0};
    double height{1080.0};

    // Text content
    std::string text_content;
    std::string font_id;
    double font_size{72.0};
    std::string alignment{"left"};
    std::map<std::string, AnimatedScalarSpec> font_axes;

    // Colors
    AnimatedColorSpec fill_color{ColorSpec{255, 255, 255, 255}};
    AnimatedColorSpec stroke_color{ColorSpec{0, 0, 0, 0}};
    double stroke_width{0.0};

    // Text-specific properties
    std::vector<TextAnimatorSpec> text_animators;
    std::vector<TextHighlightSpec> text_highlights;

    // Subtitles
    std::string subtitle_path;
    AnimatedColorSpec subtitle_outline_color;
    double subtitle_outline_width{2.0};

    // Word timestamps for text highlight animation
    std::string word_timestamp_path;

    // Layout options
    text::TextLayoutOptions layout_options;
};

// Convert TextLayerSpec to LayerSpec (adapter in presets domain)
LayerSpec make_layer_from_text_spec(const TextLayerSpec& spec);

// Build TextLayerSpec from TextParams
TextLayerSpec build_text_spec(const presets::TextParams& p);

} // namespace tachyon
