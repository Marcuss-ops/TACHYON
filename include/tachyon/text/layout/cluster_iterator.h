#pragma once

#include <cstdint>
#include <vector>
#include <cstddef>

namespace tachyon::text {

struct GraphemeCluster {
    std::size_t start_index; // index in codepoint sequence
    std::size_t length;      // number of codepoints
};

/**
 * @brief Grapheme cluster iterator with partial UAX#29 coverage.
 * 
 * Note: This implementation provides basic grapheme cluster segmentation
 * (e.g., handling combining marks, ZWJ sequences). It does NOT implement
 * the full, exhaustive UAX#29 rules for all Unicode scripts and complex
 * emoji sequences.
 */
class ClusterIterator {
public:
    static std::vector<GraphemeCluster> segment(const std::vector<std::uint32_t>& codepoints) {
        std::vector<GraphemeCluster> clusters;
        if (codepoints.empty()) return clusters;

        std::size_t start = 0;
        for (std::size_t i = 1; i < codepoints.size(); ++i) {
            if (is_break_opportunity(codepoints[i-1], codepoints[i])) {
                clusters.push_back({start, i - start});
                start = i;
            }
        }
        clusters.push_back({start, codepoints.size() - start});
        return clusters;
    }

private:
    static bool is_break_opportunity(std::uint32_t prev, std::uint32_t next) {
        // Grapheme cluster break rules (partial UAX#29 coverage)
        
        // GB3: CRLF
        if (prev == 0x000D && next == 0x000A) return false;
        
        // GB4, GB5: Control, LF, CR
        if (is_control(prev) || is_control(next)) return true;

        // GB6, GB7, GB8: Hangul Syllables
        if (is_hangul_l(prev) && (is_hangul_l(next) || is_hangul_v(next) || is_hangul_lv(next) || is_hangul_lvt(next))) return false;
        if ((is_hangul_v(prev) || is_hangul_lv(prev)) && (is_hangul_v(next) || is_hangul_t(next))) return false;
        if ((is_hangul_t(prev) || is_hangul_lvt(prev)) && is_hangul_t(next)) return false;

        // GB9: Extend, ZWJ
        if (is_extend(next) || next == 0x200D) return false;
        
        // GB9a: SpacingMark
        if (is_spacing_mark(next)) return false;

        // GB9b: Prepend
        if (is_prepend(prev)) return false;

        // GB11: ZWJ + Extended_Pictographic
        if (prev == 0x200D && is_pictographic(next)) return false;

        // GB12, GB13: Regional Indicators
        if (is_regional_indicator(prev) && is_regional_indicator(next)) {
            // Full RI parity tracking is not implemented here, so paired indicators stay together.
            return false; 
        }

        return true;
    }

private:
    static bool is_pictographic(std::uint32_t cp) {
        // Narrow Extended_Pictographic coverage for common emoji blocks.
        return (cp >= 0x1F300 && cp <= 0x1F6FF) || // Misc Symbols & Pictographs, Emoticons, Transport & Map Symbols
               (cp >= 0x1F900 && cp <= 0x1F9FF) || // Supplemental Symbols and Pictographs
               (cp >= 0x1FA70 && cp <= 0x1FAFF);   // Symbols and Pictographs Extended-A
    }

private:
    static bool is_control(std::uint32_t cp) {
        return (cp <= 0x001F) || (cp >= 0x007F && cp <= 0x009F);
    }

    static bool is_extend(std::uint32_t cp) {
        // Combining marks, variation selectors, etc.
        return (cp >= 0x0300 && cp <= 0x036F) || // Combining Diacritical Marks
               (cp >= 0x0483 && cp <= 0x0489) || // Combining Cyrillic
               (cp >= 0x0591 && cp <= 0x05BD) || // Hebrew accents
               (cp >= 0x05BF && cp <= 0x05C7) || // Hebrew marks
               (cp >= 0x0610 && cp <= 0x061A) || // Arabic marks
               (cp >= 0x064B && cp <= 0x065F) || // Arabic diacritics
               (cp >= 0x06D6 && cp <= 0x06DC) || // Arabic small high letters
               (cp >= 0x1AB0 && cp <= 0x1AFF) || // Combining Diacritical Marks Extended
               (cp >= 0x1DC0 && cp <= 0x1DFF) || // Combining Diacritical Marks Supplement
               (cp >= 0x20D0 && cp <= 0x20FF) || // Combining Diacritical Marks for Symbols
               (cp >= 0xFE20 && cp <= 0xFE2F) || // Combining Half Marks
               (cp >= 0xFE00 && cp <= 0xFE0F) || // Variation Selectors
               (cp >= 0xE0100 && cp <= 0xE01EF);  // Variation Selectors Supplement
    }

    static bool is_spacing_mark(std::uint32_t cp) {
        // SpacingMark coverage for common Indic scripts and Thai.
        return (cp >= 0x0900 && cp <= 0x0902) || cp == 0x0903 || 
               (cp >= 0x093B && cp <= 0x093F) || (cp >= 0x0941 && cp <= 0x0948) ||
               (cp >= 0x0E30 && cp <= 0x0E3A) || cp == 0x0E47 || (cp >= 0x0E4E && cp <= 0x0E4F);
    }

    static bool is_prepend(std::uint32_t cp) {
        return cp == 0x0600 || cp == 0x0601 || cp == 0x0602 || cp == 0x0603 || cp == 0x0604 || cp == 0x0605 || cp == 0x061C || cp == 0x06DD || cp == 0x070F || cp == 0x08E2 || cp == 0x110BD;
    }

    static bool is_regional_indicator(std::uint32_t cp) {
        return cp >= 0x1F1E6 && cp <= 0x1F1FF;
    }

    static bool is_hangul_l(std::uint32_t cp) { return cp >= 0x1100 && cp <= 0x115F; }
    static bool is_hangul_v(std::uint32_t cp) { return cp >= 0x1160 && cp <= 0x11A7; }
    static bool is_hangul_t(std::uint32_t cp) { return cp >= 0x11A8 && cp <= 0x11FF; }
    static bool is_hangul_lv(std::uint32_t cp) {
        if (cp < 0xAC00 || cp > 0xD7A3) return false;
        return (cp - 0xAC00) % 28 == 0;
    }
    static bool is_hangul_lvt(std::uint32_t cp) {
        if (cp < 0xAC00 || cp > 0xD7A3) return false;
        return (cp - 0xAC00) % 28 != 0;
    }
};

} // namespace tachyon::text
