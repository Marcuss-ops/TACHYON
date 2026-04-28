#include "tachyon/text/fonts/utils/font_coverage_reporter.h"
#include "tachyon/text/fonts/management/font_manifest.h"
#include "tachyon/text/fonts/core/font_face.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace tachyon::text {

FontCoverageReporter::FontCoverageReporter(const FontManifest& manifest)
    : m_manifest(manifest) {}

FontCoverageSummary FontCoverageReporter::generate_report() const {
    FontCoverageSummary summary;
    summary.total_fonts = m_manifest.fonts.size();

    for (const auto& entry : m_manifest.fonts) {
        GlyphCoverageReport report;
        report.font_family = entry.family;
        report.font_path = entry.src;

        FontFace face;
        face.set_info({
            entry.family,
            "",
            entry.weight,
            entry.style,
            entry.stretch,
            entry.src,
            entry.format,
            entry.metrics_override,
            entry.unicode_ranges,
            entry.is_fallback
        });

        if (face.load_from_file(entry.src)) {
            report.loaded = true;
            report.total_glyphs = 0;
            
            std::unordered_set<std::uint32_t> codepoints;
            face.collect_codepoints(codepoints);
            report.total_glyphs = codepoints.size();

            for (auto required_cp : m_required_codepoints) {
                if (!face.has_glyph(required_cp)) {
                    report.missing_codepoints.push_back(required_cp);
                }
            }
        } else {
            report.loaded = false;
        }

        report.has_all_required = report.missing_codepoints.empty();
        summary.total_missing_glyphs += report.missing_codepoints.size();
        summary.font_reports.push_back(std::move(report));
    }

    return summary;
}

void FontCoverageReporter::add_required_codepoints(const std::vector<std::uint32_t>& codepoints) {
    for (auto cp : codepoints) {
        m_required_codepoints.insert(cp);
    }
}

void FontCoverageReporter::add_required_range(std::uint32_t start, std::uint32_t end) {
    for (std::uint32_t cp = start; cp <= end; ++cp) {
        m_required_codepoints.insert(cp);
    }
}

std::vector<std::uint32_t> FontCoverageReporter::get_missing_for_font(const std::string& font_src) const {
    for (const auto& entry : m_manifest.fonts) {
        if (entry.src == font_src) {
            FontFace face;
            if (!face.load_from_file(entry.src)) {
                return {};
            }

            std::vector<std::uint32_t> missing;
            for (auto cp : m_required_codepoints) {
                if (!face.has_glyph(cp)) {
                    missing.push_back(cp);
                }
            }
            return missing;
        }
    }
    return {};
}

} // namespace tachyon::text

