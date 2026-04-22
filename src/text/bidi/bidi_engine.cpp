#include "tachyon/text/i18n/bidi_engine.h"

namespace tachyon::text {

namespace {

CharacterDirection resolve_strong_direction(std::uint32_t codepoint) {
    if ((codepoint >= 0x0590 && codepoint <= 0x08FF) || // Hebrew, Arabic, Syriac, Arabic Ext
        (codepoint >= 0xFB50 && codepoint <= 0xFDFF) || // Arabic Pres Forms-A
        (codepoint >= 0xFE70 && codepoint <= 0xFEFF) || // Arabic Pres Forms-B
        (codepoint >= 0x10800 && codepoint <= 0x10FFF)) { // Imperial Aramaic, Phoenician, etc.
        return CharacterDirection::RTL;
    }
    if ((codepoint >= 0x0041 && codepoint <= 0x005A) || // Latin Upper
        (codepoint >= 0x0061 && codepoint <= 0x007A) || // Latin Lower
        (codepoint >= 0x00C0 && codepoint <= 0x02AF) || // Latin Ext
        (codepoint >= 0x0370 && codepoint <= 0x052F) || // Greek, Cyrillic
        (codepoint >= 0x4E00 && codepoint <= 0x9FFF)) { // CJK Ideographs (treated as LTR)
        return CharacterDirection::LTR;
    }
    return CharacterDirection::Neutral;
}

CharacterDirection resolve_neutral(
    const std::vector<CharacterDirection>& resolved,
    std::size_t index,
    CharacterDirection base_direction) {

    for (std::size_t left = index; left-- > 0;) {
        if (resolved[left] != CharacterDirection::Neutral) {
            for (std::size_t right = index + 1; right < resolved.size(); ++right) {
                if (resolved[right] != CharacterDirection::Neutral) {
                    return resolved[left] == resolved[right] ? resolved[left] : base_direction;
                }
            }
            return resolved[left];
        }
    }

    for (std::size_t right = index + 1; right < resolved.size(); ++right) {
        if (resolved[right] != CharacterDirection::Neutral) {
            return resolved[right];
        }
    }

    return base_direction;
}

} // namespace

CharacterDirection BidiEngine::get_direction(std::uint32_t codepoint) {
    return resolve_strong_direction(codepoint);
}

std::vector<BidiRun> BidiEngine::analyze(const std::vector<std::uint32_t>& codepoints) {
    if (codepoints.empty()) return {};

    std::vector<BidiRun> all_runs;
    
    // 1. Paragraph Segmentation
    std::size_t paragraph_start = 0;
    for (std::size_t i = 0; i <= codepoints.size(); ++i) {
        bool is_para_break = (i == codepoints.size()) || (codepoints[i] == '\n' || codepoints[i] == '\r');
        if (is_para_break) {
            const std::size_t length = i - paragraph_start;
            if (length > 0 || i < codepoints.size()) {
                // Process paragraph
                auto para_runs = analyze_paragraph(codepoints, paragraph_start, length);
                all_runs.insert(all_runs.end(), para_runs.begin(), para_runs.end());
            }
            paragraph_start = i + 1;
        }
    }

    return all_runs;
}

std::vector<BidiRun> BidiEngine::analyze_paragraph(const std::vector<std::uint32_t>& codepoints, std::size_t start, std::size_t length) {
    if (length == 0) {
        // Return a single LTR run for empty paragraph (containing just the newline if any)
        return {{start, 0, CharacterDirection::LTR, 0}};
    }

    std::vector<BidiRun> runs;
    std::vector<CharacterDirection> resolved_dirs(length, CharacterDirection::Neutral);
    
    // 2. Base Direction Detection
    CharacterDirection base_direction = CharacterDirection::LTR;
    for (std::size_t i = 0; i < length; ++i) {
        const CharacterDirection dir = get_direction(codepoints[start + i]);
        if (dir != CharacterDirection::Neutral) {
            base_direction = dir;
            break;
        }
    }

    // 3. Classify each codepoint
    for (std::size_t i = 0; i < length; ++i) {
        resolved_dirs[i] = get_direction(codepoints[start + i]);
    }

    // 4. Resolve Neutrals
    for (std::size_t i = 0; i < length; ++i) {
        if (resolved_dirs[i] == CharacterDirection::Neutral) {
            // Simple neutral resolution
            resolved_dirs[i] = resolve_neutral(resolved_dirs, i, base_direction);
        }
    }

    // 5. Build Runs
    CharacterDirection current_run_dir = resolved_dirs.front();
    std::size_t run_start = 0;
    for (std::size_t i = 1; i < length; ++i) {
        if (resolved_dirs[i] != current_run_dir) {
            int level = (current_run_dir == CharacterDirection::RTL) ? 1 : 0;
            runs.push_back({start + run_start, i - run_start, current_run_dir, level});
            run_start = i;
            current_run_dir = resolved_dirs[i];
        }
    }

    int level = (current_run_dir == CharacterDirection::RTL) ? 1 : 0;
    runs.push_back({start + run_start, length - run_start, current_run_dir, level});

    return runs;
}

} // namespace tachyon::text
