#pragma once

#include "tachyon/text/fonts/font_face.h"
#include "tachyon/text/fonts/font_manifest.h"

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace tachyon::text {

struct GlyphCoverageReport {
    std::string font_family;
    std::filesystem::path font_path;
    bool loaded{false};
    std::size_t total_glyphs{0};
    std::vector<std::uint32_t> missing_codepoints;
    std::vector<std::string> missing_ranges;
    bool has_all_required{false};
};

struct FontCoverageSummary {
    std::vector<GlyphCoverageReport> font_reports;
    std::vector<std::uint32_t> all_missing_codepoints;
    std::vector<std::string> fallback_usage;
    std::size_t total_fonts{0};
    std::size_t total_missing_glyphs{0};
};

class FontCoverageReporter {
public:
    explicit FontCoverageReporter(const FontManifest& manifest);

    FontCoverageSummary generate_report() const;

    void add_required_codepoints(const std::vector<std::uint32_t>& codepoints);
    void add_required_range(std::uint32_t start, std::uint32_t end);

    std::vector<std::uint32_t> get_missing_for_font(const std::string& font_src) const;

private:
    FontManifest m_manifest;
    std::unordered_set<std::uint32_t> m_required_codepoints;
};

} // namespace tachyon::text
