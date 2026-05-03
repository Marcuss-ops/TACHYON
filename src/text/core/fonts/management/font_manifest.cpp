#include "tachyon/text/fonts/management/font_manifest.h"

#include <fstream>
#include <iterator>
#include <string>

namespace tachyon::text {

FontStyle FontManifestParser::parse_font_style(const std::string& style_str) {
    if (style_str == "italic") return FontStyle::Italic;
    if (style_str == "oblique") return FontStyle::Oblique;
    return FontStyle::Normal;
}

FontStretch FontManifestParser::parse_font_stretch(const std::string& stretch_str) {
    if (stretch_str == "ultra-condensed") return FontStretch::UltraCondensed;
    if (stretch_str == "extra-condensed") return FontStretch::ExtraCondensed;
    if (stretch_str == "condensed") return FontStretch::Condensed;
    if (stretch_str == "semi-condensed") return FontStretch::SemiCondensed;
    if (stretch_str == "semi-expanded") return FontStretch::SemiExpanded;
    if (stretch_str == "expanded") return FontStretch::Expanded;
    if (stretch_str == "extra-expanded") return FontStretch::ExtraExpanded;
    if (stretch_str == "ultra-expanded") return FontStretch::UltraExpanded;
    return FontStretch::Normal;
}

MissingGlyphPolicy FontManifestParser::parse_glyph_policy(const std::string& policy_str) {
    if (policy_str == "error") return MissingGlyphPolicy::Error;
    if (policy_str == "tofu") return MissingGlyphPolicy::Tofu;
    if (policy_str == "skip") return MissingGlyphPolicy::Skip;
    return MissingGlyphPolicy::Fallback;
}

MissingFontPolicy FontManifestParser::parse_font_policy(const std::string& policy_str) {
    if (policy_str == "warn-and-default") return MissingFontPolicy::WarnAndDefault;
    if (policy_str == "fallback") return MissingFontPolicy::Fallback;
    return MissingFontPolicy::Error;
}

std::optional<FontManifest> FontManifestParser::parse_file(const std::filesystem::path& /*path*/) {
    // Font manifest parsing has been removed.
    // Font manifests should be provided via C++ or simpler text format.
    return std::nullopt;
}

} // namespace tachyon::text
