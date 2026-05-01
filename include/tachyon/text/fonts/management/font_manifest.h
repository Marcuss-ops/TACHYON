#pragma once

#include "tachyon/text/fonts/core/font_face.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tachyon::text {

struct FontManifestEntry {
    std::string id;
    std::string family;
    std::filesystem::path src;
    std::uint32_t weight{400};
    FontStyle style{FontStyle::Normal};
    FontStretch stretch{FontStretch::Normal};
    std::string format;
    std::optional<FontMetricsOverride> metrics_override;
    std::vector<std::string> unicode_ranges;
    std::vector<std::string> subsets;
    bool is_fallback{false};
};

struct FontFallbackConfig {
    std::string primary_family;
    std::vector<std::string> fallback_families;
};

enum class MissingGlyphPolicy {
    Fallback,
    Error,
    Tofu,
    Skip
};

enum class MissingFontPolicy {
    Error,
    WarnAndDefault,
    Fallback
};

struct FontManifestPolicy {
    MissingFontPolicy missing_font{MissingFontPolicy::Error};
    MissingGlyphPolicy missing_glyph{MissingGlyphPolicy::Fallback};
    bool fail_on_missing_font{true};
    bool preload_fonts{true};
};

struct FontManifest {
    std::vector<FontManifestEntry> fonts;
    std::vector<FontFallbackConfig> fallbacks;
    FontManifestPolicy policy;
};

class FontManifestParser {
public:
    static std::optional<FontManifest> parse_file(const std::filesystem::path& path);
    static std::optional<FontManifest> parse_json(const std::string& json_content);

private:
    static FontStyle parse_font_style(const std::string& style_str);
    static FontStretch parse_font_stretch(const std::string& stretch_str);
    static MissingGlyphPolicy parse_glyph_policy(const std::string& policy_str);
    static MissingFontPolicy parse_font_policy(const std::string& policy_str);
};

} // namespace tachyon::text

