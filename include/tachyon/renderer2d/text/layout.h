#include "tachyon/renderer2d/text/layout.h"

#include <vector>

namespace tachyon::renderer2d {

struct FontContext {
  const text::Font* font;
  float font_size;
  float letter_spacing;
  float word_spacing;
  float line_height;
  float max_width;
  bool is_rtl;
  bool wrap_enabled;
};

struct GlyphRun {
  const text::Font* font;
  float font_size;
  float start_x;
  float start_y;
  std::vector<uint32_t> glyph_indices;
  std::vector<float> x_advances;
};

struct LineInfo {
  float width;
  float height;
  float offset_x;
  float offset_y;
  std::vector<GlyphRun> runs;
};

struct TextLayoutResult {
  std::vector<LineInfo> lines;
  float total_width;
  float total_height;
};

}  // namespace tachyon::renderer2d