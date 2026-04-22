#pragma once

#include <cstdint>
#include <vector>
#include <cstddef>

namespace tachyon::text {

enum class LineBreakType {
    Mandatory, // \n, \r
    Opportunity, // Space, Hyphen
    CJK,        // Between Ideographs
};

struct LineBreakOpportunity {
    std::size_t codepoint_index;
    LineBreakType type;
};

/**
 * @brief Unicode line breaking with heuristic coverage.
 * 
 * Note: This is a heuristic approach based on UAX#14. It handles
 * whitespace, explicit hyphens, and some CJK ideographs. It does NOT
 * support complex dictionary-based breaking (e.g., for Thai/Khmer) or
 * full East Asian width property rules.
 */
class LineBreaker {
public:
    static std::vector<LineBreakOpportunity> find_opportunities(const std::vector<std::uint32_t>& codepoints) {
        std::vector<LineBreakOpportunity> opportunities;
        if (codepoints.empty()) return opportunities;

        for (std::size_t i = 0; i < codepoints.size(); ++i) {
            std::uint32_t cp = codepoints[i];
            
            // Mandatory breaks
            if (cp == '\n' || cp == '\r' || cp == 0x2028 || cp == 0x2029) {
                opportunities.push_back({i, LineBreakType::Mandatory});
                continue;
            }

            // Standard space
            if (cp == ' ' || cp == '\t' || cp == 0x1680 || (cp >= 0x2000 && cp <= 0x200A) || cp == 0x202F || cp == 0x205F || cp == 0x3000) {
                opportunities.push_back({i, LineBreakType::Opportunity});
                continue;
            }

            // Hyphens
            if (cp == '-' || cp == 0x00AD || cp == 0x2010 || cp == 0x2012 || cp == 0x2013) {
                opportunities.push_back({i, LineBreakType::Opportunity});
                continue;
            }

            // CJK characters
            if ((cp >= 0x4E00 && cp <= 0x9FFF) || // CJK Unified Ideographs
                (cp >= 0x3040 && cp <= 0x309F) || // Hiragana
                (cp >= 0x30A0 && cp <= 0x30FF) || // Katakana
                (cp >= 0xAC00 && cp <= 0xD7AF)) { // Hangul Syllables
                opportunities.push_back({i, LineBreakType::CJK});
                continue;
            }
        }

        return opportunities;
    }
};

} // namespace tachyon::text
