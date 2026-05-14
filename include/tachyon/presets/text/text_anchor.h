#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/objects/text_box_spec.h"
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

[[nodiscard]] inline HorizontalAlign horizontal_align_for(TextAnchor anchor) {
    switch (anchor) {
        case TextAnchor::TopLeft:
        case TextAnchor::MiddleLeft:
        case TextAnchor::BottomLeft:
            return HorizontalAlign::Left;
        case TextAnchor::TopRight:
        case TextAnchor::MiddleRight:
        case TextAnchor::BottomRight:
            return HorizontalAlign::Right;
        case TextAnchor::TopCenter:
        case TextAnchor::MiddleCenter:
        case TextAnchor::BottomCenter:
            return HorizontalAlign::Center;
        default:
            return HorizontalAlign::Left;
    }
}

[[nodiscard]] inline VerticalAlign vertical_align_for(TextAnchor anchor) {
    switch (anchor) {
        case TextAnchor::TopLeft:
        case TextAnchor::TopCenter:
        case TextAnchor::TopRight:
            return VerticalAlign::Top;
        case TextAnchor::MiddleLeft:
        case TextAnchor::MiddleCenter:
        case TextAnchor::MiddleRight:
            return VerticalAlign::Middle;
        case TextAnchor::BottomLeft:
        case TextAnchor::BottomCenter:
        case TextAnchor::BottomRight:
            return VerticalAlign::Bottom;
        default:
            return VerticalAlign::Top;
    }
}

inline void apply_text_anchor(TextParams& params, TextAnchor anchor) {
    if (anchor == TextAnchor::None) {
        return;
    }

    params.horizontal_align = horizontal_align_for(anchor);
    params.vertical_align = vertical_align_for(anchor);

    const auto half_w = params.text_w * 0.5f;
    const auto half_h = params.text_h * 0.5f;

    switch (anchor) {
        case TextAnchor::TopLeft:
            params.x = 0.0f;
            params.y = 0.0f;
            break;
        case TextAnchor::TopCenter:
            params.x = half_w;
            params.y = 0.0f;
            break;
        case TextAnchor::TopRight:
            params.x = params.text_w;
            params.y = 0.0f;
            break;
        case TextAnchor::MiddleLeft:
            params.x = 0.0f;
            params.y = half_h;
            break;
        case TextAnchor::MiddleCenter:
            params.x = half_w;
            params.y = half_h;
            break;
        case TextAnchor::MiddleRight:
            params.x = params.text_w;
            params.y = half_h;
            break;
        case TextAnchor::BottomLeft:
            params.x = 0.0f;
            params.y = params.text_h;
            break;
        case TextAnchor::BottomCenter:
            params.x = half_w;
            params.y = params.text_h;
            break;
        case TextAnchor::BottomRight:
            params.x = params.text_w;
            params.y = params.text_h;
            break;
        case TextAnchor::None:
            break;
    }
}

inline void apply_text_anchor(LayerSpec& spec, TextAnchor anchor) {
    if (anchor == TextAnchor::None) {
        return;
    }

    spec.text.box.horizontal_align = horizontal_align_for(anchor);
    spec.text.box.vertical_align = vertical_align_for(anchor);

    const auto half_w = static_cast<float>(spec.transform.width) * 0.5f;
    const auto half_h = static_cast<float>(spec.transform.height) * 0.5f;

    switch (anchor) {
        case TextAnchor::TopLeft:
            spec.transform.transform.position_x = 0.0f;
            spec.transform.transform.position_y = 0.0f;
            break;
        case TextAnchor::TopCenter:
            spec.transform.transform.position_x = half_w;
            spec.transform.transform.position_y = 0.0f;
            break;
        case TextAnchor::TopRight:
            spec.transform.transform.position_x = static_cast<float>(spec.transform.width);
            spec.transform.transform.position_y = 0.0f;
            break;
        case TextAnchor::MiddleLeft:
            spec.transform.transform.position_x = 0.0f;
            spec.transform.transform.position_y = half_h;
            break;
        case TextAnchor::MiddleCenter:
            spec.transform.transform.position_x = half_w;
            spec.transform.transform.position_y = half_h;
            break;
        case TextAnchor::MiddleRight:
            spec.transform.transform.position_x = static_cast<float>(spec.transform.width);
            spec.transform.transform.position_y = half_h;
            break;
        case TextAnchor::BottomLeft:
            spec.transform.transform.position_x = 0.0f;
            spec.transform.transform.position_y = static_cast<float>(spec.transform.height);
            break;
        case TextAnchor::BottomCenter:
            spec.transform.transform.position_x = half_w;
            spec.transform.transform.position_y = static_cast<float>(spec.transform.height);
            break;
        case TextAnchor::BottomRight:
            spec.transform.transform.position_x = static_cast<float>(spec.transform.width);
            spec.transform.transform.position_y = static_cast<float>(spec.transform.height);
            break;
        case TextAnchor::None:
            break;
    }
}

} // namespace tachyon::presets::text
