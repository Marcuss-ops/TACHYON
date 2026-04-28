#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <unordered_set>

namespace tachyon::text {

enum class FontStyle : std::uint8_t {
    Normal,
    Italic,
    Oblique
};

enum class FontStretch : std::uint8_t {
    UltraCondensed,
    ExtraCondensed,
    Condensed,
    SemiCondensed,
    Normal,
    SemiExpanded,
    Expanded,
    ExtraExpanded,
    UltraExpanded
};

struct FontMetricsOverride {
    std::optional<float> ascent;
    std::optional<float> descent;
    std::optional<float> line_gap;
    std::optional<float> line_height;
};

struct FontFaceInfo {
    std::string family;
    std::string style_name;
    std::uint32_t weight{400};
    FontStyle style{FontStyle::Normal};
    FontStretch stretch{FontStretch::Normal};
    std::filesystem::path path;
    std::string format;
    std::optional<FontMetricsOverride> metrics_override;
    std::vector<std::string> unicode_ranges;
    bool is_fallback{false};
};

class FontFace {
public:
    FontFace();
    ~FontFace();

    FontFace(const FontFace&) = delete;
    FontFace& operator=(const FontFace&) = delete;
    FontFace(FontFace&& other) noexcept;
    FontFace& operator=(FontFace&& other) noexcept;

    bool load_from_file(const std::filesystem::path& path);

    const FontFaceInfo& info() const { return m_info; }
    void set_info(const FontFaceInfo& info) { m_info = info; }

    bool is_loaded() const { return m_loaded; }
    bool is_freetype() const { return m_is_freetype; }

    void* freetype_face() const { return m_ft_face; }
    const std::vector<std::uint8_t>& font_data() const { return m_font_data; }

    std::uint64_t id() const { return m_id; }
    std::uint64_t content_hash() const { return m_content_hash; }

    bool has_glyph(std::uint32_t codepoint) const;
    std::uint32_t glyph_index_for_codepoint(std::uint32_t codepoint) const;

    std::int32_t get_kerning(std::uint32_t left, std::uint32_t right) const;

    std::int32_t ascent() const { return m_ascent; }
    std::int32_t descent() const { return m_descent; }
    std::int32_t line_height() const { return m_line_height; }
    std::int32_t default_advance() const { return m_default_advance; }

    void collect_codepoints(std::unordered_set<std::uint32_t>& codepoints) const;

private:
    static std::uint64_t compute_content_hash(const std::vector<std::uint8_t>& data);

    std::uint64_t m_id;
    std::uint64_t m_content_hash{0};
    bool m_loaded{false};
    bool m_is_freetype{false};

    FontFaceInfo m_info;

    std::int32_t m_ascent{0};
    std::int32_t m_descent{0};
    std::int32_t m_line_height{0};
    std::int32_t m_default_advance{0};

    void* m_ft_face{nullptr};
    std::vector<std::uint8_t> m_font_data;

    static std::atomic<std::uint64_t> next_font_id;
};

} // namespace tachyon::text
