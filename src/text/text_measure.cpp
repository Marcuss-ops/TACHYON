#include "tachyon/text/layout/layout.h"
#include "tachyon/text/fonts/core/font_registry.h"

#include <string_view>

namespace tachyon::text {

struct TextMeasureResult {
    float width{0.0f};
    float height{0.0f};
};

/**
 * Measure text bounds (Remotion measureText).
 * Returns the {width, height} for a given text string, font, and size.
 */
inline TextMeasureResult measure_text(FontRegistry& registry, std::string_view text, const std::string& font_name, float pixel_size) {
    TextMeasureResult result;
    
    // Setup style
    TextStyle style;
    style.pixel_size = static_cast<std::uint32_t>(pixel_size);
    
    // Setup text box (unbounded)
    TextBox text_box;
    text_box.multiline = false;
    
    // Layout options
    TextLayoutOptions layout_options;
    
    // Call layout
    auto layout_result = layout_text(
        registry,
        font_name,
        text,
        style,
        text_box,
        TextAlignment::Left,
        layout_options
    );
    
    result.width = layout_result.total_bounds.width;
    result.height = layout_result.total_bounds.height;
    
    return result;
}

}  // namespace tachyon::text

