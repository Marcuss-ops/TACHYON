#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/presets/text/text_params.h"

#include <string>
#include <string_view>

namespace tachyon::presets::text {

enum class TextAnchor {
    None,
    TopLeft,
    TopCenter,
    TopRight,
    MiddleLeft,
    MiddleCenter,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
};

[[nodiscard]] inline std::string_view alignment_for(TextAnchor anchor) {
    switch (anchor) {
        case TextAnchor::TopLeft:
        case TextAnchor::MiddleLeft:
        case TextAnchor::BottomLeft:
            return "left";
        case TextAnchor::TopRight:
        case TextAnchor::MiddleRight:
        case TextAnchor::BottomRight:
            return "right";
        case TextAnchor::TopCenter:
        case TextAnchor::MiddleCenter:
        case TextAnchor::BottomCenter:
            return "center";
        case TextAnchor::None:
            return "";
    }
    return "";
}

template <typename T>
inline void apply_text_anchor_common(T& spec, TextAnchor anchor, float text_w, float text_h) {
    if (anchor == TextAnchor::None) {
        return;
    }

    const auto alignment = alignment_for(anchor);
    if (!alignment.empty()) {
        spec.alignment = std::string(alignment);
    }

    const auto half_w = text_w * 0.5f;
    const auto half_h = text_h * 0.5f;

    switch (anchor) {
        case TextAnchor::TopLeft:
            spec.x = 0.0f;
            spec.y = 0.0f;
            break;
        case TextAnchor::TopCenter:
            spec.x = half_w;
            spec.y = 0.0f;
            break;
        case TextAnchor::TopRight:
            spec.x = text_w;
            spec.y = 0.0f;
            break;
        case TextAnchor::MiddleLeft:
            spec.x = 0.0f;
            spec.y = half_h;
            break;
        case TextAnchor::MiddleCenter:
            spec.x = half_w;
            spec.y = half_h;
            break;
        case TextAnchor::MiddleRight:
            spec.x = text_w;
            spec.y = half_h;
            break;
        case TextAnchor::BottomLeft:
            spec.x = 0.0f;
            spec.y = text_h;
            break;
        case TextAnchor::BottomCenter:
            spec.x = half_w;
            spec.y = text_h;
            break;
        case TextAnchor::BottomRight:
            spec.x = text_w;
            spec.y = text_h;
            break;
        case TextAnchor::None:
            break;
    }
}

inline void apply_text_anchor(TextParams& params, TextAnchor anchor) {
    apply_text_anchor_common(params, anchor, params.text_w, params.text_h);
}

inline void apply_text_anchor(LayerSpec& spec, TextAnchor anchor) {
    apply_text_anchor_common(spec, anchor, static_cast<float>(spec.width), static_cast<float>(spec.height));
}

} // namespace tachyon::presets::text
