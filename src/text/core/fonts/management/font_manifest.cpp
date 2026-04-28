#include "tachyon/text/fonts/management/font_manifest.h"
#include <nlohmann/json.hpp>

#include <fstream>
#include <iterator>
#include <string>

namespace tachyon::text {

using json = nlohmann::json;

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

std::optional<FontManifest> FontManifestParser::parse_file(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::nullopt;
    }

    const std::string content((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    return parse_json(content);
}

std::optional<FontManifest> FontManifestParser::parse_json(const std::string& json_content) {
    try {
        const json j = json::parse(json_content);
        FontManifest manifest;

        if (j.contains("fonts")) {
            for (const auto& font_json : j["fonts"]) {
                FontManifestEntry entry;
                entry.id = font_json.value("id", "");
                entry.family = font_json.value("family", "");
                entry.src = font_json.value("src", "");
                entry.weight = font_json.value("weight", 400);

                if (font_json.contains("style")) {
                    entry.style = parse_font_style(font_json["style"]);
                }
                if (font_json.contains("stretch")) {
                    entry.stretch = parse_font_stretch(font_json["stretch"]);
                }
                if (font_json.contains("format")) {
                    entry.format = font_json["format"];
                }
                if (font_json.contains("isFallback")) {
                    entry.is_fallback = font_json["isFallback"];
                }

                if (font_json.contains("metricsOverride")) {
                    FontMetricsOverride override;
                    if (font_json["metricsOverride"].contains("ascent")) {
                        override.ascent = font_json["metricsOverride"]["ascent"];
                    }
                    if (font_json["metricsOverride"].contains("descent")) {
                        override.descent = font_json["metricsOverride"]["descent"];
                    }
                    if (font_json["metricsOverride"].contains("lineGap")) {
                        override.line_gap = font_json["metricsOverride"]["lineGap"];
                    }
                    if (font_json["metricsOverride"].contains("lineHeight")) {
                        override.line_height = font_json["metricsOverride"]["lineHeight"];
                    }
                    entry.metrics_override = override;
                }

                if (font_json.contains("unicodeRanges")) {
                    for (const auto& range : font_json["unicodeRanges"]) {
                        entry.unicode_ranges.push_back(range);
                    }
                }

                manifest.fonts.push_back(std::move(entry));
            }
        }

        if (j.contains("fontFallbacks")) {
            for (auto& [primary, fallbacks] : j["fontFallbacks"].items()) {
                FontFallbackConfig config;
                config.primary_family = primary;
                for (const auto& fallback : fallbacks) {
                    config.fallback_families.push_back(fallback);
                }
                manifest.fallbacks.push_back(std::move(config));
            }
        }

        if (j.contains("policy")) {
            const auto& policy_json = j["policy"];
            if (policy_json.contains("missingFont")) {
                manifest.policy.missing_font = parse_font_policy(policy_json["missingFont"]);
            }
            if (policy_json.contains("missingGlyph")) {
                manifest.policy.missing_glyph = parse_glyph_policy(policy_json["missingGlyph"]);
            }
            if (policy_json.contains("failOnMissingFont")) {
                manifest.policy.fail_on_missing_font = policy_json["failOnMissingFont"];
            }
            if (policy_json.contains("preloadFonts")) {
                manifest.policy.preload_fonts = policy_json["preloadFonts"];
            }
        }

        return manifest;
    } catch (const json::exception&) {
        return std::nullopt;
    }
}

} // namespace tachyon::text

